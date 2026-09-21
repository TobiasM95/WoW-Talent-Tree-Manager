#!/usr/bin/env python3
"""Tests for icon name handling and fetching.

    python services/ingest/tests/test_icons.py

No network: the fetch tests drive a fake opener. What is being tested is the *classification*
of upstream responses, which is the part that has consequences -- a 404 cached as absence is
correct, a 503 cached as absence blanks icons until someone clears the cache by hand.
"""
from __future__ import annotations

import io
import os
import sys
import urllib.error

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

from ttm_ingest import icons as I  # noqa: E402

_failures = []


def check(name, condition, detail=""):
    print(f"{'ok  ' if condition else 'FAIL'} {name}"
          f"{(' -- ' + detail) if detail and not condition else ''}")
    if not condition:
        _failures.append(name)


class FakeResponse(io.BytesIO):
    def __init__(self, payload, content_type="image/jpeg", status=200):
        super().__init__(payload)
        self.status = status
        self.headers = {"Content-Type": content_type}

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()
        return False


class FakeOpener:
    """Returns a queued response, or raises a queued error, per call."""

    def __init__(self, *outcomes):
        self.outcomes = list(outcomes)
        self.urls = []

    def open(self, request, timeout=None):  # noqa: ARG002
        self.urls.append(request.full_url)
        outcome = self.outcomes.pop(0) if self.outcomes else self.outcomes
        if isinstance(outcome, Exception):
            raise outcome
        return outcome


def http_error(code):
    return urllib.error.HTTPError("http://x", code, "nope", {}, None)


# ---------------------------------------------------------------------------
# names
# ---------------------------------------------------------------------------

def test_names():
    check("a plain name passes through",
          I.normalise("ability_druid_mangle") == "ability_druid_mangle")
    check("case and whitespace are normalised",
          I.normalise("  Ability_Druid_Mangle ") == "ability_druid_mangle")

    # The live payload really does contain this. The icon behind it exists, so stripping
    # the extension recovers a working name rather than dropping the icon.
    check("a raw asset filename loses its extension",
          I.normalise("ability_druid_mangle.tga") == "ability_druid_mangle")
    check(".jpg is stripped too, so /icons/foo.jpg and /icons/foo agree",
          I.normalise("spell_frost_frostbolt.jpg") == "spell_frost_frostbolt")

    for bad in ("../../etc/passwd", "spell/frost", "spell frost", "", "a",
                "x" * 200, "Ünicode"):
        try:
            I.normalise(bad)
            check(f"rejects {bad!r}", False, "no error raised")
        except I.IconNameError:
            check(f"rejects {bad!r}", True)

    # Distinct exception types, because the caller's advice differs: one says "re-run to
    # retry" and for the other that would be a lie.
    check("a name error is an icon error too",
          issubclass(I.IconNameError, I.IconError))


def test_urls():
    check("url has size and extension",
          I.icon_url("spell_x", 56).endswith("/56/spell_x.jpg"))
    check("base url is honoured",
          I.icon_url("spell_x", 36, "https://mirror.example/i")
          == "https://mirror.example/i/36/spell_x.jpg")
    check("a trailing slash on the base does not double up",
          I.icon_url("spell_x", 18, "https://mirror.example/i/")
          == "https://mirror.example/i/18/spell_x.jpg")
    try:
        I.icon_url("spell_x", 64)
        check("an unsupported size is rejected", False, "no error raised")
    except I.IconError:
        check("an unsupported size is rejected", True)


def test_icon_names_from_definition():
    definition = {"nodes": [
        {"entries": [{"icon": "One"}, {"icon": "two.tga"}]},
        {"entries": [{"icon": None}, {"icon": "two"}, {}]},
        {"entries": [{"icon": "not a name"}]},
    ]}
    names = I.icon_names(definition)
    check("names come from entries, normalised and deduplicated",
          names == {"one", "two"}, repr(names))


# ---------------------------------------------------------------------------
# fetching
# ---------------------------------------------------------------------------

def test_fetch_success():
    payload = b"\xff\xd8" + b"x" * 2000
    opener = FakeOpener(FakeResponse(payload))
    row = I.fetch("spell_x", 56, opener=opener)
    check("a good response becomes a storable row",
          row["status"] == 200 and row["bytes"] == payload
          and row["content_type"] == "image/jpeg", str(row)[:120])
    check("the etag is a content hash", row["etag"] == I.etag_for(payload))
    check("the source url is recorded", row["source"].endswith("/56/spell_x.jpg"))
    check("charset is stripped from the content type",
          I.fetch("spell_x", 56,
                  opener=FakeOpener(FakeResponse(payload, "image/jpeg; charset=binary"))
                  )["content_type"] == "image/jpeg")


def test_fetch_absence_is_data():
    """4xx is an answer, and caching it is what stops the sync asking forever."""
    for code in (403, 404):
        row = I.fetch("spell_x", 56, opener=FakeOpener(http_error(code)))
        check(f"{code} becomes a negative cache row",
              row["status"] == code and row["bytes"] is None, str(row)[:100])
        check(f"{code} row carries no etag or content type",
              row["etag"] is None and row["content_type"] is None, str(row)[:100])


def test_fetch_transient_failures_raise():
    """5xx and network errors must not be recorded as absence.

    Caching a 503 as "this icon does not exist" would blank icons for a month, and nothing
    would look broken enough for anyone to investigate.
    """
    for outcome, label in ((http_error(503), "503"),
                           (http_error(500), "500"),
                           (urllib.error.URLError("dns"), "network error")):
        try:
            I.fetch("spell_x", 56, opener=FakeOpener(outcome))
            check(f"{label} raises rather than caching absence", False, "no error raised")
        except I.IconNameError:
            check(f"{label} raises rather than caching absence", False,
                  "raised IconNameError, which would be reported as unfixable")
        except I.IconError:
            check(f"{label} raises rather than caching absence", True)


def test_fetch_rejects_implausible_bodies():
    """A 200 that is not an image must not poison the cache.

    An upstream that answers with an HTML error page and status 200 is the case that makes
    this worth checking: the bytes would store fine and every client would fail to decode
    them, with nothing in the pipeline reporting a problem.
    """
    cases = [
        (FakeResponse(b"<html>nope</html>", "text/html"), "an HTML error page"),
        (FakeResponse(b"tiny"), "a body too small to be an icon"),
        (FakeResponse(b"x" * (I.MAX_BYTES + 1)), "a body too large to be an icon"),
    ]
    for response, label in cases:
        try:
            I.fetch("spell_x", 56, opener=FakeOpener(response))
            check(f"rejects {label}", False, "no error raised")
        except I.IconError:
            check(f"rejects {label}", True)


def test_fetch_many_separates_the_unfixable():
    payload = b"\xff\xd8" + b"x" * 2000

    class PerName:
        def open(self, request, timeout=None):  # noqa: ARG002
            if "good" in request.full_url:
                return FakeResponse(payload)
            raise urllib.error.URLError("dns")

    rows, failed, unusable = I.fetch_many(
        ["good_one", "bad_network", "not a name"], 56, workers=2, opener=PerName())

    check("good names produce rows", [r["name"] for r in rows] == ["good_one"], repr(rows))
    check("network failures are retryable", failed == ["bad_network"], repr(failed))
    check("bad names are reported as unfixable", unusable == ["not a name"], repr(unusable))


def main():
    test_names()
    test_urls()
    test_icon_names_from_definition()
    test_fetch_success()
    test_fetch_absence_is_data()
    test_fetch_transient_failures_raise()
    test_fetch_rejects_implausible_bodies()
    test_fetch_many_separates_the_unfixable()

    print()
    if _failures:
        print(f"{len(_failures)} failed: {', '.join(_failures)}")
        return 1
    print("all icon tests passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
