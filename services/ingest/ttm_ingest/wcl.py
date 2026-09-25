"""What top players actually run, from WarcraftLogs' public API (v2).

The client-credentials flow reads public data -- rankings, reports -- with no user login and so
no redirect URL, which is what lets a local service use it without a public server. The keys
come from the environment (WCL_CLIENT_ID, WCL_CLIENT_SECRET) and never leave the API.

**Rankings carry talents directly.** With `includeCombatantInfo`, each ranked player comes with
`talents: [{talentID, points}]`, and a talentID is a *trait entry* id -- so it names not only
the node but, for a choice node, which alternative was taken. Checked against Blood Death
Knight: 7,699 talent picks from 100 players all landed on our trees, and the only two ids that
did not were the hero-tree selector's entries, splitting 74/26 exactly as the hero trees did.

Granted talents are in the list too (a class tree reads 35 against a 34-point cap). They cost
nothing and are not part of a build, so they are dropped on the way in.

Budget: 3,600 points an hour; a rankings page of 100 players costs one or two. Results are
cached by the caller, so a panel opened by many people costs one query per spec and fight.
"""

from __future__ import annotations

import base64
import json
import os
import threading
import time
import urllib.parse
import urllib.request
from typing import Any

TOKEN_URL = "https://www.warcraftlogs.com/oauth/token"
API_URL = "https://www.warcraftlogs.com/api/v2/client"
UA = "ttm-talent-tree-manager (+https://github.com/TobiasM95/WoW-Talent-Tree-Manager)"

# WarcraftLogs difficulty ids.
DIFFICULTIES = {"mythic": 5, "heroic": 4, "normal": 3, "dungeon": 10}


class WclError(Exception):
    pass


class NotConfigured(WclError):
    pass


_token: dict[str, Any] = {}
_lock = threading.Lock()


def configured() -> bool:
    return bool(os.environ.get("WCL_CLIENT_ID") and os.environ.get("WCL_CLIENT_SECRET"))


def _access_token() -> str:
    with _lock:
        if _token and _token["expires"] > time.time() + 60:
            return _token["value"]
        cid, secret = os.environ.get("WCL_CLIENT_ID"), os.environ.get("WCL_CLIENT_SECRET")
        if not (cid and secret):
            raise NotConfigured("WarcraftLogs is not configured: set WCL_CLIENT_ID and WCL_CLIENT_SECRET")
        auth = base64.b64encode(f"{cid}:{secret}".encode()).decode()
        request = urllib.request.Request(
            TOKEN_URL,
            data=urllib.parse.urlencode({"grant_type": "client_credentials"}).encode(),
            headers={"Authorization": f"Basic {auth}", "User-Agent": UA},
        )
        try:
            with urllib.request.urlopen(request, timeout=30) as response:
                body = json.load(response)
        except urllib.error.HTTPError as exc:
            raise WclError(f"WarcraftLogs refused the credentials ({exc.code})") from None
        _token.update(value=body["access_token"], expires=time.time() + int(body.get("expires_in", 3600)))
        return _token["value"]


def gql(query: str, variables: dict[str, Any] | None = None) -> dict[str, Any]:
    body = json.dumps({"query": query, "variables": variables or {}}).encode()
    request = urllib.request.Request(
        API_URL,
        data=body,
        headers={"Authorization": f"Bearer {_access_token()}", "Content-Type": "application/json", "User-Agent": UA},
    )
    try:
        with urllib.request.urlopen(request, timeout=60) as response:
            out = json.load(response)
    except urllib.error.HTTPError as exc:
        raise WclError(f"WarcraftLogs answered {exc.code}") from None
    if out.get("errors"):
        raise WclError(out["errors"][0].get("message", "query failed"))
    return out["data"]


def current_content() -> list[dict[str, Any]]:
    """The newest expansion's live raids and Mythic+ seasons, with their encounters.

    "Live" is WarcraftLogs' own `frozen: false`, and PTR, beta and aggregate "complete raid"
    zones are left out: they either rank nothing yet or rank the same fights twice.
    """
    data = gql(
        "{ worldData { expansions { id name zones { id name frozen "
        "difficulties { id name } encounters { id name } } } } }"
    )
    expansions = sorted(data["worldData"]["expansions"], key=lambda x: x["id"], reverse=True)
    out = []
    for zone in expansions[0]["zones"] if expansions else []:
        name = zone["name"]
        if zone["frozen"] or not zone["encounters"]:
            continue
        if any(word in name for word in ("PTR", "Beta", "Complete Raid", "Dummy")):
            continue
        dungeon = "Mythic+" in name
        out.append({
            "zoneId": zone["id"],
            "name": name,
            "kind": "dungeon" if dungeon else "raid",
            "difficulties": (
                [{"id": 10, "name": "Mythic+"}]
                if dungeon
                else [d for d in zone["difficulties"] if d["id"] in (5, 4)]
            ),
            "encounters": zone["encounters"],
        })
    return out


def wcl_name(text: str) -> str:
    """WarcraftLogs spells classes and specs without spaces: "Death Knight" -> "DeathKnight"."""
    return text.replace(" ", "")


def rankings(class_name: str, spec_name: str, encounter: int, difficulty: int, pages: int = 1,
             metric: str = "dps") -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    query = (
        "query($c:String!,$s:String!,$e:Int!,$d:Int,$p:Int,$m:CharacterRankingMetricType){"
        " worldData { encounter(id:$e) { characterRankings(className:$c, specName:$s,"
        " difficulty:$d, page:$p, metric:$m, includeCombatantInfo:true) } } }"
    )
    for page in range(1, pages + 1):
        data = gql(query, {"c": wcl_name(class_name), "s": wcl_name(spec_name), "e": encounter,
                           "d": difficulty, "p": page, "m": metric})
        ranked = data["worldData"]["encounter"]["characterRankings"] or {}
        if ranked.get("error"):
            raise WclError(ranked["error"])
        rows.extend(ranked.get("rankings") or [])
        if not ranked.get("hasMorePages"):
            break
    return rows


def entry_index(trees: list[dict[str, Any]]) -> dict[int, tuple[str, dict[str, Any], int]]:
    """Every trait entry id in a spec's trees -> (tree key, node, which alternative)."""
    out = {}
    for tree in trees:
        for node in tree["nodes"]:
            for i, entry in enumerate(node.get("entries") or []):
                if entry.get("entryId") is not None:
                    out[int(entry["entryId"])] = (tree["key"], node, i)
    return out


def convert(row: dict[str, Any], index: dict[int, tuple[str, dict[str, Any], int]]) -> dict[str, Any]:
    """One ranked player as a build in our terms: points per tree, choice sides, hero tree."""
    points: dict[str, dict[str, int]] = {}
    choices: dict[str, int] = {}
    unknown: list[int] = []
    for talent in row.get("talents") or []:
        hit = index.get(int(talent["talentID"]))
        if hit is None:
            unknown.append(int(talent["talentID"]))
            continue
        key, node, side = hit
        if node.get("preFilled"):
            continue  # granted: part of the character, not of the build
        tree = points.setdefault(key, {})
        nid = str(node["nodeId"])
        if node["kind"] == "choice" and len(node.get("entries") or []) >= 2:
            # A choice node's entries are alternatives: one is taken, and it says which.
            tree[nid] = int(talent["points"])
            choices[nid] = side
        else:
            # Anything else accumulates. A tiered node reports one entry per rank -- Blood's
            # 4-rank capstone arrives as four entries of one point -- and taking the last one
            # instead of the sum dropped three points from every Blood build, which the
            # legality check could not see: a build spending *less* is still legal.
            tree[nid] = tree.get(nid, 0) + int(talent["points"])
    return {
        "name": row.get("name"),
        "amount": row.get("amount"),
        "report": (row.get("report") or {}).get("code"),
        "points": points,
        "choices": choices,
        "unknown": unknown,
    }


def problems(tree: dict[str, Any], build: dict[str, int], cap: int | None) -> str | None:
    """Why a build breaks our copy of a tree's rules, or None.

    Real builds from the live game are the best test our tree data has: if one of them is
    "illegal" here, our data is wrong or stale, not the player.
    """
    nodes = {str(n["nodeId"]): n for n in tree["nodes"]}
    granted = {k for k, n in nodes.items() if n.get("preFilled")}
    spent = sum(build.values())
    if cap is not None and spent > cap:
        return f"spends {spent}, over the cap of {cap}"
    for key, rank in build.items():
        node = nodes.get(key)
        if node is None:
            return f"talent {key} is not in the tree"
        if rank > node["maxPoints"]:
            return f"{node['name']} at {rank} of {node['maxPoints']}"
    # Gates: a talent needs its gate spent before it -- in the tree, in some valid order.
    # Points are placed gate-first, which is the order any valid spending can be rearranged into.
    order = sorted(build, key=lambda k: nodes[k]["pointsRequired"])
    so_far = 0
    for key in order:
        node = nodes[key]
        if so_far < node["pointsRequired"]:
            return f"{node['name']} needs {node['pointsRequired']} spent before it, has {so_far}"
        so_far += build[key]
    for key in build:
        parents = [str(p) for p in nodes[key]["parents"] if str(p) in nodes]
        if parents and not any(p in granted or build.get(p, 0) >= nodes[p]["maxPoints"] for p in parents):
            return f"{nodes[key]['name']} without a full-rank parent"
    return None
