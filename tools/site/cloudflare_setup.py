#!/usr/bin/env python3
"""First-release setup on Cloudflare, idempotent: the Pages project and the KV namespace.

    CLOUDFLARE_API_TOKEN=... CLOUDFLARE_ACCOUNT_ID=... \\
        python3 tools/site/cloudflare_setup.py --project talent-tree-manager --toml frontend/wrangler.toml

Talks to Cloudflare's REST API directly rather than parsing wrangler's printed output, so a
refusal comes back as Cloudflare's own message -- which permission the token lacks, say --
instead of an empty variable three commands later.

On the first release it creates what is missing; afterwards it finds what is there and changes
nothing. It writes the namespace's id into wrangler.toml, whose committed id is a placeholder.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import sys
import urllib.error
import urllib.request

API = "https://api.cloudflare.com/client/v4"


class Refused(Exception):
    pass


def call(method: str, path: str, token: str, body: dict | None = None) -> dict:
    request = urllib.request.Request(
        API + path, method=method,
        data=json.dumps(body).encode() if body is not None else None,
        headers={"Authorization": f"Bearer {token}", "Content-Type": "application/json"},
    )
    try:
        with urllib.request.urlopen(request, timeout=60) as response:
            out = json.load(response)
    except urllib.error.HTTPError as exc:
        try:
            out = json.load(exc)
        except Exception:
            raise Refused(f"{method} {path}: HTTP {exc.code}") from None
    if not out.get("success"):
        messages = "; ".join(f"{e.get('code')}: {e.get('message')}" for e in out.get("errors", [])) or "no detail"
        raise Refused(f"{method} {path}: {messages}")
    return out


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--project", required=True)
    parser.add_argument("--branch", default="master")
    parser.add_argument("--toml", required=True)
    args = parser.parse_args()
    token = os.environ.get("CLOUDFLARE_API_TOKEN", "")
    account = os.environ.get("CLOUDFLARE_ACCOUNT_ID", "")
    if not token or not account:
        print("CLOUDFLARE_API_TOKEN and CLOUDFLARE_ACCOUNT_ID must both be set (repository secrets).")
        return 1

    try:
        # Which token this is and that it is active. Its permissions show up below, per call.
        status = call("GET", "/user/tokens/verify", token)["result"].get("status")
        print(f"token: {status}")

        try:
            call("GET", f"/accounts/{account}/pages/projects/{args.project}", token)
            print(f"Pages project '{args.project}': exists")
        except Refused as exc:
            if "8000007" not in str(exc) and "not found" not in str(exc).lower():
                raise
            call("POST", f"/accounts/{account}/pages/projects", token,
                 {"name": args.project, "production_branch": args.branch})
            print(f"Pages project '{args.project}': created")

        title = f"{args.project}-projects"
        found = None
        page = 1
        while found is None:
            out = call("GET", f"/accounts/{account}/storage/kv/namespaces?per_page=100&page={page}", token)
            found = next((n for n in out["result"] if n["title"] == title), None)
            info = out.get("result_info") or {}
            if found or page >= (info.get("total_pages") or 1):
                break
            page += 1
        if found is None:
            found = call("POST", f"/accounts/{account}/storage/kv/namespaces", token, {"title": title})["result"]
            print(f"KV namespace '{title}': created")
        else:
            print(f"KV namespace '{title}': exists")
    except Refused as exc:
        print(f"Cloudflare refused: {exc}")
        print("The API token needs: Account > Cloudflare Pages > Edit, and Account > Workers KV Storage > Edit.")
        return 1

    with open(args.toml, encoding="utf-8") as handle:
        text = handle.read()
    text, n = re.subn(r'^id = "[^"]*"', f'id = "{found["id"]}"', text, count=1, flags=re.M)
    if n != 1:
        print(f"no KV id line in {args.toml}")
        return 1
    with open(args.toml, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(text)
    print(f"wrangler.toml: KV namespace {found['id']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
