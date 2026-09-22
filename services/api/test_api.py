#!/usr/bin/env python3
"""API tests, run against a live instance.

    docker compose up -d api
    python services/api/test_api.py [base_url]

Uses only the standard library, so it needs nothing installed locally. Asserts behaviour
and invariants, not exact counts -- those are pinned by the DP and engine suites.
"""
from __future__ import annotations

import json
import sys
import urllib.error
import urllib.request

BASE = sys.argv[1].rstrip("/") if len(sys.argv) > 1 else "http://localhost:8001"

_failures: list[str] = []
_run = 0


def check(name, fn):
    global _run
    _run += 1
    try:
        fn()
        print(f"  [ok  ] {name}")
    except AssertionError as exc:
        _failures.append(name)
        print(f"  [FAIL] {name}: {exc}")
    except Exception as exc:  # noqa: BLE001
        _failures.append(name)
        print(f"  [FAIL] {name}: unexpected {type(exc).__name__}: {exc}")


def get(path):
    with urllib.request.urlopen(BASE + path, timeout=60) as r:
        return json.loads(r.read())


def post(path, body, expect=200):
    data = json.dumps(body).encode()
    req = urllib.request.Request(
        BASE + path, data=data, headers={"Content-Type": "application/json"}
    )
    try:
        with urllib.request.urlopen(req, timeout=120) as r:
            assert r.status == expect, f"expected {expect}, got {r.status}"
            return json.loads(r.read())
    except urllib.error.HTTPError as exc:
        assert exc.code == expect, f"expected {expect}, got {exc.code}: {exc.read()[:200]}"
        return json.loads(exc.read() or b"{}")


SPEC = "retail/11/102/spec"
HERO = "retail/11/102/hero/23"
# Blood Death Knight: its spec tree ends in a detached, gate-only capstone.
SPEC_WITH_CAPSTONE = "retail/6/250/spec"
# Feral Druid's class tree: Rake, Rip and Swipe are granted to it and not to its siblings.
CLASS_WITH_GRANTED = "retail/11/103/class"


def t_health_reports_promoted_data():
    h = get("/health")
    assert h["status"] == "ok", h
    assert h["trees"] > 0 and h["nodes"] > 0, h
    # Data age is the thing the legacy pipeline never surfaced.
    assert "dataAgeSeconds" in h, h


def t_trees_listing_covers_every_kind():
    trees = get("/trees")
    kinds = {t["kind"] for t in trees}
    assert {"class", "spec", "hero"} <= kinds, kinds
    assert len(trees) >= 160, len(trees)


def t_trees_listing_filters():
    trees = get("/trees?classId=11&specId=102")
    assert len(trees) == 4, trees          # class + spec + two hero
    assert sum(1 for t in trees if t["kind"] == "hero") == 2


def t_tree_definition_has_graph_and_text():
    tree = get(f"/trees/{SPEC}")
    assert tree["nodes"], "no nodes"
    assert any(n["parents"] for n in tree["nodes"]), "no edges"
    assert any(n["entries"][0]["ranks"] for n in tree["nodes"]), "no descriptions"


def t_precomputed_counts_are_monotone_in_nothing_but_present():
    rows = get(f"/trees/{SPEC}/counts")
    assert len(rows) > 10, len(rows)
    for r in rows:
        # A set with k choice nodes expands to 2^k builds, so builds >= sets always.
        assert r["builds"] >= r["sets"], r


def t_unfiltered_count_is_precomputed():
    r = post("/counts", {"treeKey": SPEC, "points": 20})
    assert r["source"] == "precomputed", r
    assert r["filtered"] is False
    assert r["builds"] >= r["sets"] > 0


def t_filtered_count_is_computed_and_never_wider():
    """A filter can only remove builds, never add them.

    Note it does not always *reduce* the count: the top rows of a spec tree are taken by
    every build at a realistic budget (excluding Eclipse at 20 points yields zero), so
    requiring them narrows nothing. That is a feature of the data, and it is precisely the
    "requiring this changes nothing" signal the marginals give a user.
    """
    tree = get(f"/trees/{SPEC}")
    plain = [n["nodeId"] for n in tree["nodes"] if n["kind"] != "choice"]
    base = post("/counts", {"treeKey": SPEC, "points": 20})
    narrowed = post("/counts", {"treeKey": SPEC, "points": 20, "mustHave": plain[:3]})
    assert narrowed["source"] == "computed", narrowed
    assert narrowed["filtered"] is True
    assert narrowed["sets"] <= base["sets"], (narrowed["sets"], base["sets"])


def t_an_optional_talent_strictly_narrows():
    """Requiring a genuinely optional talent must reduce the count."""
    tree = get(f"/trees/{SPEC}")
    base = post("/counts", {"treeKey": SPEC, "points": 20})["sets"]
    optional = None
    for node in tree["nodes"]:
        if node["kind"] == "choice":
            continue
        excluded = post("/counts", {"treeKey": SPEC, "points": 20,
                                    "mustNotHave": [node["nodeId"]]})["sets"]
        if 0 < excluded < base:
            optional = node["nodeId"]
            break
    assert optional is not None, "expected at least one optional talent"
    required = post("/counts", {"treeKey": SPEC, "points": 20,
                                "mustHave": [optional]})["sets"]
    assert required < base, (required, base)


def t_require_and_exclude_partition_the_space():
    """The complement property, over HTTP: every build either takes the node or does not."""
    tree = get(f"/trees/{SPEC}")
    node = [n["nodeId"] for n in tree["nodes"] if n["kind"] != "choice"][5]
    base = post("/counts", {"treeKey": SPEC, "points": 16})["sets"]
    inc = post("/counts", {"treeKey": SPEC, "points": 16, "mustHave": [node]})["sets"]
    exc = post("/counts", {"treeKey": SPEC, "points": 16, "mustNotHave": [node]})["sets"]
    assert inc + exc == base, f"{inc} + {exc} != {base}"


def t_choice_sides_partition_the_space():
    tree = get(f"/trees/{SPEC}")
    choice = next(n["nodeId"] for n in tree["nodes"] if n["kind"] == "choice")
    base = post("/counts", {"treeKey": SPEC, "points": 14})["builds"]
    total = 0
    for side in ("a", "b", "none"):
        total += post("/counts", {"treeKey": SPEC, "points": 14,
                                  "choiceSides": {str(choice): side}})["builds"]
    assert total == base, f"{total} != {base}"


def t_gate_flips_on_result_size():
    wide = post("/counts", {"treeKey": SPEC, "points": 30})
    assert wide["listable"] is False, "a 17M-set result should not be offered for listing"
    tree = get(f"/trees/{SPEC}")
    plain = [n["nodeId"] for n in tree["nodes"] if n["kind"] != "choice"]
    narrow = post("/counts", {"treeKey": SPEC, "points": 14, "mustHave": plain[:3]})
    assert narrow["listable"] is True, narrow


def t_counts_are_fast_enough_to_type_against():
    """The gate is only useful if it can run while a user paints constraints."""
    tree = get(f"/trees/{SPEC}")
    plain = [n["nodeId"] for n in tree["nodes"] if n["kind"] != "choice"]
    r = post("/counts", {"treeKey": SPEC, "points": 30, "mustHave": plain[:2]})
    assert r["elapsedMs"] < 500, f"filtered count took {r['elapsedMs']}ms"


def t_unknown_node_is_rejected():
    r = post("/counts", {"treeKey": SPEC, "points": 10, "mustHave": [999999]}, expect=400)
    assert "not in" in str(r.get("detail", "")), r


def t_contradictory_filter_is_rejected():
    tree = get(f"/trees/{SPEC}")
    node = tree["nodes"][0]["nodeId"]
    r = post("/counts", {"treeKey": SPEC, "points": 10,
                         "mustHave": [node], "mustNotHave": [node]}, expect=400)
    assert "both required and excluded" in str(r.get("detail", "")), r


def t_budget_beyond_the_tree_is_rejected():
    r = post("/counts", {"treeKey": HERO, "points": 30}, expect=400)
    assert "point slots" in str(r.get("detail", "")), r


def t_unknown_tree_is_404():
    post("/counts", {"treeKey": "nope/not/real", "points": 10}, expect=404)


# --- group filters ---------------------------------------------------------

def _group(points=14, size=3):
    tree = get(f"/trees/{SPEC}")
    return [n["nodeId"] for n in tree["nodes"]
            if n["kind"] != "choice" and n["pointsRequired"] >= 8][:size]


def t_at_least_one_of_is_the_complement():
    """at-least-one(G) + none-of(G) == unconstrained. Same identity the DP asserts."""
    group = _group()
    base = post("/counts", {"treeKey": SPEC, "points": 14})["sets"]
    at_least = post("/counts", {"treeKey": SPEC, "points": 14,
                                "atLeastOneOf": [group]})["sets"]
    none = post("/counts", {"treeKey": SPEC, "points": 14, "mustNotHave": group})["sets"]
    assert at_least + none == base, f"{at_least} + {none} != {base}"


def t_exactly_one_is_narrower_than_at_least_one():
    group = _group()
    at_least = post("/counts", {"treeKey": SPEC, "points": 14,
                                "atLeastOneOf": [group]})["sets"]
    exactly = post("/counts", {"treeKey": SPEC, "points": 14,
                               "exactlyOneOf": [group]})["sets"]
    assert 0 < exactly < at_least, (exactly, at_least)


def t_group_request_counts_as_filtered():
    r = post("/counts", {"treeKey": SPEC, "points": 14, "atLeastOneOf": [_group()]})
    assert r["filtered"] is True and r["source"] == "computed", r


def t_single_node_group_is_rejected():
    tree = get(f"/trees/{SPEC}")
    one = tree["nodes"][0]["nodeId"]
    r = post("/counts", {"treeKey": SPEC, "points": 10, "atLeastOneOf": [[one]]},
             expect=400)
    assert "at least two nodes" in str(r.get("detail", "")), r


def t_listing_refuses_more_groups_than_the_engine_can_express():
    """Counting handles many groups; the engine's filter holds one value per talent, so
    listing supports one group of each kind. Say so rather than dropping constraints."""
    group = _group(size=4)
    a, b = group[:2], group[2:]
    # counting two groups is fine
    post("/counts", {"treeKey": SPEC, "points": 14, "atLeastOneOf": [a, b]})
    # listing them is not
    r = post("/solve", {"treeKey": SPEC, "points": 14, "atLeastOneOf": [a, b]}, expect=400)
    assert "at most one" in str(r.get("detail", "")), r


def t_solved_builds_actually_satisfy_the_group():
    """The engine's -3 sentinel must mean what the count meant."""
    group = _group()
    job = post("/solve", {"treeKey": SPEC, "points": 14, "exactlyOneOf": [group]},
               expect=202)
    done = _await_job(job["id"])
    assert done["state"] == "done", done
    assert done["resultCount"] == done["expectedCount"], done
    page = get(f"/solve/{job['id']}/results?limit=200")
    for build in page["builds"]:
        present = [g for g in group if str(g) in build]
        assert len(present) == 1, f"build holds {len(present)} of the group: {present}"


# --- solve jobs (need a running worker) ------------------------------------

def _raw_request(path, headers=None):
    """(status, headers, body) for a request that may legitimately be a 304 or a 4xx.

    The headers are returned as the message object, not a dict: HTTP header names are
    case-insensitive and dict() makes them case-sensitive, so `.get("ETag")` would miss a
    header spelled `etag` on the wire.
    """
    req = urllib.request.Request(BASE + path, headers=headers or {})
    try:
        with urllib.request.urlopen(req, timeout=60) as r:
            return r.status, r.headers, r.read()
    except urllib.error.HTTPError as exc:
        return exc.code, exc.headers, exc.read()


def get_raw(path):
    """Like get(), but returns (status, body) instead of raising on 4xx."""
    try:
        with urllib.request.urlopen(BASE + path, timeout=60) as r:
            return r.status, json.loads(r.read())
    except urllib.error.HTTPError as exc:
        return exc.code, json.loads(exc.read() or b"{}")


def _queue_a_cancellable_job():
    """A job big enough to still be running when the cancel arrives.

    Just under the listing limit: the engine finishes in well under a second, so what the
    cancel lands in is the storing phase. That is the realistic case anyway -- storing is
    where a large job actually spends its wall clock.
    """
    return post("/solve", {"treeKey": SPEC, "points": 25}, expect=202)


def _await_job(job_id, timeout=120):
    import time
    deadline = time.time() + timeout
    while time.time() < deadline:
        job = get(f"/solve/{job_id}")
        if job["state"] in ("done", "capped", "failed", "cancelled"):
            return job
        time.sleep(0.5)
    raise AssertionError(f"job {job_id} did not finish within {timeout}s")


def t_solve_produces_exactly_the_predicted_count():
    """The gate and the engine must agree.

    This is the property that lets the UI promise a number before the work happens: the
    worker refuses a result that contradicts the pre-flight count.
    """
    tree = get(f"/trees/{SPEC}")
    plain = [n["nodeId"] for n in tree["nodes"] if n["kind"] != "choice"]
    job = post("/solve", {"treeKey": SPEC, "points": 12, "mustHave": plain[3:6]},
               expect=202)
    done = _await_job(job["id"])
    assert done["state"] == "done", done
    assert done["resultCount"] == done["expectedCount"], done


def t_solve_results_are_nodeid_keyed_and_spend_the_budget():
    tree = get(f"/trees/{SPEC}")
    plain = [n["nodeId"] for n in tree["nodes"] if n["kind"] != "choice"]
    job = post("/solve", {"treeKey": SPEC, "points": 11, "mustHave": plain[3:6]},
               expect=202)
    _await_job(job["id"])
    page = get(f"/solve/{job['id']}/results?limit=50")
    assert page["builds"], "no builds returned"
    valid = {str(n["nodeId"]) for n in tree["nodes"]}
    for build in page["builds"]:
        assert set(build) <= valid, "a build references an unknown node id"
        assert sum(build.values()) == 11, f"build spends {sum(build.values())}, not 11"


def t_oversized_solve_is_refused_before_queueing():
    r = post("/solve", {"treeKey": SPEC, "points": 30}, expect=413)
    assert "exceeds the limit" in str(r.get("detail", "")), r


def t_impossible_solve_is_refused():
    tree = get(f"/trees/{SPEC}")
    # requiring a talent that cannot be reached within the budget matches nothing
    deep = [n["nodeId"] for n in tree["nodes"] if n["pointsRequired"] >= 20]
    if not deep:
        return
    post("/solve", {"treeKey": SPEC, "points": 3, "mustHave": deep[:1]}, expect=400)


def t_identical_requests_share_one_job():
    body = {"treeKey": SPEC, "points": 9, "mustHave": [], "mustNotHave": []}
    first = post("/solve", body, expect=202)
    second = post("/solve", body, expect=202)
    assert first["id"] == second["id"], (first["id"], second["id"])


def t_results_are_not_served_before_they_exist():
    body = {"treeKey": SPEC, "points": 8}
    job = post("/solve", body, expect=202)
    done = _await_job(job["id"])
    assert done["state"] in ("done", "capped"), done
    # and a job that does not exist is a 404, not an empty page
    try:
        get("/solve/00000000-0000-0000-0000-000000000000/results")
        raise AssertionError("expected 404 for an unknown job")
    except urllib.error.HTTPError as exc:
        assert exc.code == 404, exc.code


def t_every_filter_kind_reaches_the_engine():
    """The gate and the engine must agree for *each* kind of constraint, not just some.

    This is the regression test for a real wrong-answer bug: choice sides were honoured by
    the counting DP and dropped entirely by the worker's filter string, so a request pinning
    three sides was counted as 14,795 selections and enumerated as 58,738. The job failed
    rather than serving the wrong number -- the worker's count check caught it -- but a user
    saw a filter silently do nothing.

    Every constraint kind is exercised separately, because "some filters work" is exactly
    the state that produced the bug.
    """
    tree = get(f"/trees/{SPEC}")
    plain = [n["nodeId"] for n in tree["nodes"] if n["kind"] != "choice"]
    choices = [n["nodeId"] for n in tree["nodes"] if n["kind"] == "choice"]
    assert choices, "this tree has no choice nodes to pin"

    cases = {
        "mustHave": {"mustHave": plain[3:5]},
        "mustNotHave": {"mustNotHave": plain[-3:]},
        "choiceSide a": {"choiceSides": {str(choices[0]): "a"}},
        "choiceSide b": {"choiceSides": {str(choices[0]): "b"}},
        "choiceSide none": {"choiceSides": {str(choices[0]): "none"}},
        "several sides": {
            "choiceSides": {str(c): side
                            for c, side in zip(choices[:3], ["a", "b", "none"])},
        },
        "sides plus requires": {
            "mustHave": plain[3:5],
            "choiceSides": {str(choices[0]): "a"},
        },
        "atLeastOneOf": {"atLeastOneOf": [plain[6:9]]},
        "exactlyOneOf": {"exactlyOneOf": [plain[6:8]]},
    }

    for label, extra in cases.items():
        body = {"treeKey": SPEC, "points": 11, **extra}
        predicted = post("/counts", body)["sets"]
        if predicted == 0 or predicted > 2_000_000:
            continue  # nothing to enumerate, or the gate would refuse it
        job = post("/solve", body, expect=202)
        done = _await_job(job["id"])
        assert done["state"] == "done", (label, done["state"], done["error"])
        assert done["resultCount"] == predicted, (label, done["resultCount"], predicted)


def t_granted_talents_cannot_be_constrained():
    """A talent the game gives you for free is not part of any build's point spend.

    The DP removes pre-filled roots from the graph entirely and promotes their children, so
    nothing ever spends a point on one. A constraint naming such a node used to pass
    validation and then be silently dropped: requiring it and excluding it both returned the
    unfiltered count, which means a user who barred a talent got back builds that all had it,
    with nothing anywhere to notice.

    Which talents are granted depends on the **specialisation**, not just the class -- Rake,
    Rip and Swipe come free to a Feral Druid and not to a Balance one -- so the class tree is
    per spec and the two are not interchangeable.
    """
    tree = get(f"/trees/{CLASS_WITH_GRANTED}")
    granted = [n for n in tree["nodes"] if n["preFilled"]]
    assert granted, "this class tree no longer has granted talents"
    node = granted[0]["nodeId"]

    for body in ({"mustHave": [node]}, {"mustNotHave": [node]},
                 {"atLeastOneOf": [[node, granted[-1]["nodeId"]]]}):
        r = post("/counts", {"treeKey": CLASS_WITH_GRANTED, "points": 10, **body},
                 expect=400)
        assert "granted automatically" in str(r.get("detail", "")), (body, r)

    # And an unconstrained count still works, so the check has not become a blanket refusal.
    assert post("/counts", {"treeKey": CLASS_WITH_GRANTED, "points": 10})["sets"] > 0


def t_class_trees_differ_by_specialisation():
    """One class tree per spec, and they are genuinely different.

    Only the granted talents differ, which is easy to mistake for duplication -- and picking
    whichever came first in the listing showed the wrong ones and produced a talent string
    the game would not accept.
    """
    trees = get("/trees?kind=class&classId=11")
    assert len(trees) >= 3, trees
    assert all(t["specName"] for t in trees), "class trees must name their specialisation"

    granted = {}
    for summary in trees:
        detail = get(f"/trees/{summary['key']}")
        granted[summary["specName"]] = frozenset(
            n["nodeId"] for n in detail["nodes"] if n["preFilled"]
        )
    assert all(granted.values()), granted
    assert len(set(granted.values())) > 1, (
        "every spec's class tree grants the same talents, which contradicts the data"
    )


def t_a_gated_capstone_is_reachable():
    """A talent with no edges at all, unlocked purely by the last point gate.

    Retail spec trees end in one of these -- Blood Death Knight's "Dance of Midnight" is a
    four-rank node with no parents, no children and `pointsRequired` at the final gate. It is
    structurally unlike everything else in the tree, it exists in no other version of the
    game, and it is exactly the shape that breaks quietly: a traversal that only reaches
    nodes through edges never finds it, and reports a smaller number with no error.

    So this pins the whole chain. The gate is measured *before* the point is placed, which is
    why a tree gated at 20 needs 21 points before any build contains it.
    """
    tree = get(f"/trees/{SPEC_WITH_CAPSTONE}")
    gates = sorted({n["pointsRequired"] for n in tree["nodes"]})
    capstone = next(
        (n for n in tree["nodes"]
         if not n["parents"] and not n["children"] and n["pointsRequired"] == gates[-1]),
        None,
    )
    assert capstone, "this tree no longer has a detached capstone"
    gate = capstone["pointsRequired"]

    at_gate = post("/counts", {"treeKey": SPEC_WITH_CAPSTONE, "points": gate,
                               "mustHave": [capstone["nodeId"]]})["sets"]
    assert at_gate == 0, f"{gate} points should not be enough: the gate is checked first"

    predicted = post("/counts", {"treeKey": SPEC_WITH_CAPSTONE, "points": gate + 1,
                                 "mustHave": [capstone["nodeId"]]})["sets"]
    assert predicted > 0, "no build reaches the capstone at all"

    # And the engine must find the same ones, which is the half that a traversal bug breaks.
    job = post("/solve", {"treeKey": SPEC_WITH_CAPSTONE, "points": gate + 1,
                          "mustHave": [capstone["nodeId"]]}, expect=202)
    done = _await_job(job["id"])
    assert done["state"] == "done", done
    assert done["resultCount"] == predicted, (done["resultCount"], predicted)

    page = get(f"/solve/{job['id']}/results?limit=5")
    assert page["builds"], "no builds returned"
    for build in page["builds"]:
        assert str(capstone["nodeId"]) in build, build


def t_cancelling_a_finished_job_is_refused():
    """A job that already produced results cannot be un-produced."""
    tree = get(f"/trees/{SPEC}")
    plain = [n["nodeId"] for n in tree["nodes"] if n["kind"] != "choice"]
    job = post("/solve", {"treeKey": SPEC, "points": 10, "mustHave": plain[3:6]},
               expect=202)
    done = _await_job(job["id"])
    assert done["state"] == "done", done
    body = post(f"/solve/{job['id']}/cancel", {}, expect=409)
    assert "done" in body.get("detail", ""), body


def t_cancel_is_idempotent():
    """Retrying a cancel returns the same answer instead of failing.

    A client that loses the response to its first cancel has no way to tell whether it
    landed. Making the retry an error would push that ambiguity onto the caller.
    """
    job = _queue_a_cancellable_job()
    first = post(f"/solve/{job['id']}/cancel", {}, expect=200)
    second = post(f"/solve/{job['id']}/cancel", {}, expect=200)
    assert first["cancelRequested"] and second["cancelRequested"], (first, second)
    final = _await_job(job["id"])
    assert final["state"] == "cancelled", final


def t_cancelled_job_keeps_no_results():
    """A cancelled job leaves nothing half-stored.

    The store is a single COPY, so unwinding out of it aborts the transaction -- there is
    no state in which a user can page through the fraction of a job they cancelled.
    """
    job = _queue_a_cancellable_job()
    post(f"/solve/{job['id']}/cancel", {}, expect=200)
    final = _await_job(job["id"])
    assert final["state"] == "cancelled", final
    assert final["resultCount"] in (None, 0), final
    status, _ = get_raw(f"/solve/{job['id']}/results")
    assert status == 409, status


def t_cancelled_job_does_not_claim_full_progress():
    """Progress must not read 100% for work that was abandoned."""
    job = _queue_a_cancellable_job()
    post(f"/solve/{job['id']}/cancel", {}, expect=200)
    final = _await_job(job["id"])
    assert final["state"] == "cancelled", final
    assert final["progress"] < 1.0, final["progress"]


def t_cancelling_an_unknown_job_is_404():
    post("/solve/00000000-0000-0000-0000-000000000000/cancel", {}, expect=404)


def _an_icon_name():
    tree = get(f"/trees/{SPEC}")
    for node in tree["nodes"]:
        for entry in node["entries"]:
            if entry.get("icon"):
                return entry["icon"]
    raise AssertionError("no tree entry carries an icon name")


def t_icon_is_served_with_immutable_caching():
    """An icon never changes under a name, which is the whole reason to serve them per file.

    Without `immutable` + a long max-age, a tree canvas would re-request 60 icons on every
    navigation; with it, the second visit makes no icon requests at all.
    """
    status, headers, body = _raw_request(f"/icons/{_an_icon_name()}.jpg")
    assert status == 200, status
    assert headers.get("Content-Type", "").startswith("image/"), headers
    assert "immutable" in headers.get("Cache-Control", ""), headers
    assert headers.get("ETag"), headers
    # JPEG magic. Serving something a browser cannot decode would still pass a status check.
    assert body[:2] == bytes((0xFF, 0xD8)), body[:8]


def t_icon_honours_conditional_get():
    path = f"/icons/{_an_icon_name()}"
    _, headers, _ = _raw_request(path)
    status, _, body = _raw_request(path, {"If-None-Match": headers["ETag"]})
    assert status == 304, status
    assert not body, body[:32]


def t_icon_extension_is_optional():
    """`/icons/foo` and `/icons/foo.jpg` are the same icon.

    Both spellings will be written by hand, and the name normaliser the ingest uses already
    strips the extension -- so this is a property of sharing that function, not a special case.
    """
    name = _an_icon_name()
    bare = _raw_request(f"/icons/{name}")
    with_ext = _raw_request(f"/icons/{name}.jpg")
    assert bare[0] == with_ext[0] == 200, (bare[0], with_ext[0])
    assert bare[2] == with_ext[2], "different bytes for the same icon"


def t_unknown_icon_is_404_not_an_error():
    """Upstream has no art for every name, so a miss is expected and must be survivable."""
    status, _, _ = _raw_request("/icons/spell_this_does_not_exist_at_all")
    assert status == 404, status


def t_icon_name_is_validated():
    for bad in ("/icons/not%20a%20name", "/icons/a"):
        status, _, _ = _raw_request(bad)
        assert status == 400, (bad, status)


def t_icon_size_is_validated():
    status, _, _ = _raw_request(f"/icons/{_an_icon_name()}?size=64")
    assert status == 400, status


def t_health_reports_icon_coverage():
    body = get("/health")
    coverage = body.get("iconCoverage")
    assert coverage is None or 0.0 <= coverage <= 1.0, coverage


def t_stats_agree_with_the_dp():
    """Talent frequencies must match the count the DP gives for requiring that talent.

    This is the strongest check available on the whole pipeline, because the two sides share
    nothing: one is a SQL aggregate over rows the C++ engine produced and the worker decoded,
    the other is the frontier DP answering "how many builds take this talent". They agree on
    every talent or something between the engine, the decoder and the DP is wrong.
    """
    job = post("/solve", {"treeKey": SPEC, "points": 13}, expect=202)
    done = _await_job(job["id"])
    assert done["state"] == "done", done

    stats = get(f"/solve/{job['id']}/stats")
    assert stats["talents"], "no statistics produced"
    assert stats["total"] == done["resultCount"], (stats["total"], done["resultCount"])

    for talent in stats["talents"]:
        expected = post("/counts",
                        {"treeKey": SPEC, "points": 13,
                         "mustHave": [talent["nodeId"]]})["sets"]
        assert talent["builds"] == expected, (talent["nodeId"], talent["builds"], expected)


def t_stats_shape_is_sane():
    job = post("/solve", {"treeKey": SPEC, "points": 12}, expect=202)
    _await_job(job["id"])
    stats = get(f"/solve/{job['id']}/stats")

    for talent in stats["talents"]:
        assert 0 < talent["share"] <= 1, talent
        # A talent cannot appear in more builds than exist, and cannot be taken at a lower
        # mean rank than one, since a row only exists for talents the build actually takes.
        assert talent["builds"] <= stats["total"], talent
        assert talent["meanPoints"] >= 1, talent
        assert talent["mandatory"] == (talent["builds"] == stats["total"]), talent

    # Sorted most common first, which is the order the ranked list reads in.
    shares = [t["share"] for t in stats["talents"]]
    assert shares == sorted(shares, reverse=True), shares[:5]

    # A talent taken by every matching build is one the constraints already decided. At a
    # realistic budget a spec tree always has some, and saying so is the point of the view.
    assert any(t["mandatory"] for t in stats["talents"]), "expected some mandatory talents"


def t_stats_not_served_before_results_exist():
    body = {"treeKey": SPEC, "points": 30}
    post("/counts", body)  # oversized, so no job can exist for it
    r = post("/solve", body, expect=413)
    assert r
    status, _ = get_raw("/solve/00000000-0000-0000-0000-000000000000/stats")
    assert status == 404, status


def main() -> int:
    print(f"api: {BASE}")
    for name, fn in [
        ("health reports promoted data", t_health_reports_promoted_data),
        ("tree listing covers every kind", t_trees_listing_covers_every_kind),
        ("tree listing filters by class and spec", t_trees_listing_filters),
        ("tree definition has graph and descriptions", t_tree_definition_has_graph_and_text),
        ("precomputed counts present, builds >= sets",
         t_precomputed_counts_are_monotone_in_nothing_but_present),
        ("unfiltered count comes from precomputed rows", t_unfiltered_count_is_precomputed),
        ("filtered count is computed and never wider",
         t_filtered_count_is_computed_and_never_wider),
        ("an optional talent strictly narrows", t_an_optional_talent_strictly_narrows),
        ("require + exclude partition the space", t_require_and_exclude_partition_the_space),
        ("choice sides partition the space", t_choice_sides_partition_the_space),
        ("listing gate flips on result size", t_gate_flips_on_result_size),
        ("counts fast enough to type against", t_counts_are_fast_enough_to_type_against),
        ("unknown node rejected", t_unknown_node_is_rejected),
        ("contradictory filter rejected", t_contradictory_filter_is_rejected),
        ("budget beyond the tree rejected", t_budget_beyond_the_tree_is_rejected),
        ("unknown tree is 404", t_unknown_tree_is_404),
    ]:
        check(name, fn)

    print("\ngroup filters:")
    for name, fn in [
        ("at-least-one is the complement of none-of", t_at_least_one_of_is_the_complement),
        ("exactly-one is narrower than at-least-one",
         t_exactly_one_is_narrower_than_at_least_one),
        ("a group request counts as filtered", t_group_request_counts_as_filtered),
        ("a one-node group is rejected", t_single_node_group_is_rejected),
        ("listing refuses more groups than the engine expresses",
         t_listing_refuses_more_groups_than_the_engine_can_express),
        ("solved builds actually satisfy the group",
         t_solved_builds_actually_satisfy_the_group),
    ]:
        check(name, fn)

    print("\nsolve jobs (needs a worker):")
    for name, fn in [
        ("solve produces exactly the predicted count",
         t_solve_produces_exactly_the_predicted_count),
        ("results are nodeId-keyed and spend the budget",
         t_solve_results_are_nodeid_keyed_and_spend_the_budget),
        ("oversized solve refused before queueing", t_oversized_solve_is_refused_before_queueing),
        ("impossible solve refused", t_impossible_solve_is_refused),
        ("identical requests share one job", t_identical_requests_share_one_job),
        ("results not served before they exist", t_results_are_not_served_before_they_exist),
    ]:
        check(name, fn)

    print("\nicons:")
    for name, fn in [
        ("an icon is served with immutable caching",
         t_icon_is_served_with_immutable_caching),
        ("a conditional GET gets 304", t_icon_honours_conditional_get),
        ("the .jpg extension is optional", t_icon_extension_is_optional),
        ("an unknown icon is 404", t_unknown_icon_is_404_not_an_error),
        ("an unusable icon name is 400", t_icon_name_is_validated),
        ("an unsupported size is 400", t_icon_size_is_validated),
        ("health reports icon coverage", t_health_reports_icon_coverage),
    ]:
        check(name, fn)

    print("\nstatistics:")
    for name, fn in [
        ("talent frequencies agree with the DP", t_stats_agree_with_the_dp),
        ("statistics are well formed and ranked", t_stats_shape_is_sane),
        ("statistics are not served for an unknown job",
         t_stats_not_served_before_results_exist),
    ]:
        check(name, fn)

    print("\nunusual tree shapes:")
    check("a detached, gate-only capstone is reachable", t_a_gated_capstone_is_reachable)
    check("granted talents cannot be constrained", t_granted_talents_cannot_be_constrained)
    check("class trees differ by specialisation", t_class_trees_differ_by_specialisation)

    print("\nfilters reach the engine:")
    check("every filter kind agrees between gate and engine",
          t_every_filter_kind_reaches_the_engine)

    print("\ncancellation:")
    for name, fn in [
        ("cancelling a finished job is refused", t_cancelling_a_finished_job_is_refused),
        ("cancel is idempotent", t_cancel_is_idempotent),
        ("a cancelled job keeps no results", t_cancelled_job_keeps_no_results),
        ("a cancelled job does not claim full progress",
         t_cancelled_job_does_not_claim_full_progress),
        ("cancelling an unknown job is 404", t_cancelling_an_unknown_job_is_404),
    ]:
        check(name, fn)

    print()
    if _failures:
        print(f"{len(_failures)} of {_run} FAILED: {', '.join(_failures)}")
        return 1
    print(f"all {_run} passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
