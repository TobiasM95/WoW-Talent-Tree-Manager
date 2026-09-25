#!/usr/bin/env python3
"""Tests for the worker's pure parts: no database, no queue.

    python services/worker/test_worker.py [path/to/ttm-solver]

Covers the two pieces that are easy to get silently wrong:

  - decoding the engine's positional output into nodeId-keyed builds
  - reading progress off the solver's stderr while it runs

The progress test runs the real solver if it can find one, because the only way this
breaks is a mismatch between what the engine prints and what the worker parses -- which
a mock reproduces by construction and therefore cannot catch.
"""
import os
import subprocess
import sys
import time
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import worker  # noqa: E402

PRESETS = os.path.join(REPO, "Engine", "resources", "presets.txt")

_failures = []


def check(name, condition, detail=""):
    mark = "ok  " if condition else "FAIL"
    print(f"{mark} {name}{(' -- ' + detail) if detail and not condition else ''}")
    if not condition:
        _failures.append(name)


def write(path, text):
    with open(path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(text)


# ---------------------------------------------------------------------------
# decoding
# ---------------------------------------------------------------------------

def test_decoding(tmp):
    tree = {"nodes": [{"nodeId": 100}, {"nodeId": 200}, {"nodeId": 300}]}
    path = os.path.join(tmp, "results.txt")

    # Header maps bit -> positional talent index. Bits 0 and 1 both belong to talent 0,
    # which is how a two-rank talent is represented.
    write(path, "0/0/1/2/\n3\n5\n12,2\n")
    rows = worker.decode_results(path, tree, 10)
    check("two bits on one talent become two points",
          rows[0] == {"100": 2}, repr(rows[0]))
    check("bits across talents stay separate",
          rows[1] == {"100": 1, "200": 1}, repr(rows[1]))
    check("trailing choice indices are ignored",
          rows[2] == {"200": 1, "300": 1}, repr(rows[2]))

    check("limit truncates", len(worker.decode_results(path, tree, 2)) == 2)

    write(path, "")
    check("empty output decodes to nothing", worker.decode_results(path, tree, 10) == [])

    # A header referencing a talent the tree does not have means the tree rendered for
    # the engine and the tree we are decoding against have diverged. Storing those rows
    # would silently attribute points to the wrong nodes.
    write(path, "0/1/2/9/\n1\n")
    try:
        worker.decode_results(path, tree, 10)
        check("out-of-range talent index is rejected", False, "no error raised")
    except worker.SolveFailed as exc:
        check("out-of-range talent index is rejected", "index 9" in str(exc), str(exc))

    write(path, "0/1/2/\nnot-a-number\n")
    try:
        worker.decode_results(path, tree, 10)
        check("unreadable row is rejected", False, "no error raised")
    except worker.SolveFailed:
        check("unreadable row is rejected", True)

    # iter_results is what the COPY consumes; it must agree with the list form exactly.
    write(path, "0/0/1/2/\n3\n5\n12,2\n")
    check("iter_results matches decode_results",
          list(worker.iter_results(path, tree, 10)) == worker.decode_results(path, tree, 10))


# ---------------------------------------------------------------------------
# filter strings
# ---------------------------------------------------------------------------

def test_filter_string():
    tree = {"nodes": [{"nodeId": 10}, {"nodeId": 20}, {"nodeId": 30}, {"nodeId": 40}]}
    check("must-have is 1, must-not is -1",
          worker.build_filter_string(tree, [10], [30]) == "1:0:-1:0")
    check("at-least-one group is -2",
          worker.build_filter_string(tree, [], [], [[20, 40]]) == "0:-2:0:-2")
    check("exactly-one group is -3",
          worker.build_filter_string(tree, [], [], [], [[10, 20]]) == "-3:-3:0:0")
    check("unconstrained is all zeroes",
          worker.build_filter_string(tree, [], []) == "0:0:0:0")
    check("rank limits: at least, at most, exactly",
          worker.build_filter_string(tree, [], [], rank_min={"10": 2}, rank_max={"20": 1, "30": 2})
          == "2:0/1:0/2:0")


# ---------------------------------------------------------------------------
# progress streaming
# ---------------------------------------------------------------------------

def find_solver():
    for candidate in (os.environ.get("TTM_SOLVER"),
                      "/usr/local/bin/ttm-solver",
                      os.path.join(REPO, "build", "ttm-solver"),
                      os.path.join(REPO, "build", "ttm-solver.exe")):
        if candidate and os.path.exists(candidate):
            return candidate
    return None


def test_progress_streaming(solver, tmp):
    """The solver must actually emit PROGRESS lines that the worker actually parses.

    druid_restoration at 30 points takes ~24 s and yields 305,286,987 combinations, so
    at a 250 ms interval there is no doubt about whether lines appear. --count-only
    keeps it to a counter rather than a 9.5 GB file.
    """
    seen = []
    args = [solver,
            "--structure-file-path", PRESETS,
            "--structure-indices", "2",
            "--target-talent-count", "30",
            "--count-only",
            "--max-results", "10000000000",
            "--progress", "--progress-interval-ms", "250"]
    def record(count):
        seen.append(count)
        return True

    stdout_text, stderr_tail, code, killed, stopped = worker._run_streaming(
        args, os.path.join(tmp, "out.txt"), 600, record)

    check("solver exited cleanly",
          code == 0 and not killed and not stopped,
          f"code={code} killed={killed} stopped={stopped}")
    check("progress lines were seen", len(seen) > 5, f"{len(seen)} lines")
    check("progress only increases", seen == sorted(seen), repr(seen[:5]))
    check("progress stays under the final count",
          all(n < 305_286_987 for n in seen), repr(seen[-3:]))
    check("stdout still carries the result",
          "305286987 combinations" in stdout_text, stdout_text[-120:])
    check("progress lines are not mistaken for diagnostics",
          "PROGRESS" not in stderr_tail, stderr_tail[:120])


def test_cancel_kills_the_solver(solver, tmp):
    """A cancel during the solving phase must actually stop the solver.

    Same 24-second solve as above, told to stop on the second progress line. If the kill
    did not work this would run to completion, so the elapsed time is the assertion: the
    process has to be gone long before the solve would have finished.
    """
    seen = []

    def stop_after_two(count):
        seen.append(count)
        return len(seen) < 2

    args = [solver,
            "--structure-file-path", PRESETS,
            "--structure-indices", "2",
            "--target-talent-count", "30",
            "--count-only",
            "--max-results", "10000000000",
            "--progress", "--progress-interval-ms", "250"]
    started = time.monotonic()
    stdout_text, _, _, killed, stopped = worker._run_streaming(
        args, os.path.join(tmp, "cancel.out"), 600, stop_after_two)
    elapsed = time.monotonic() - started

    check("stop request is reported", stopped and not killed,
          f"stopped={stopped} killed={killed}")
    check("solver died promptly", elapsed < 10, f"{elapsed:.1f}s (full solve is ~24s)")
    check("no final count was produced",
          "combinations" not in stdout_text, stdout_text[-120:])


def test_watchdog(tmp):
    """A solver that never exits must not hold the worker forever."""
    script = os.path.join(tmp, "hang.py")
    write(script, "import time\ntime.sleep(300)\n")
    _, _, _, killed, stopped = worker._run_streaming(
        [sys.executable, script], os.path.join(tmp, "hang.out"), 2.0, None)
    check("a hung solver is killed at the deadline", killed and not stopped,
          f"killed={killed} stopped={stopped}")


def main():
    solver = sys.argv[1] if len(sys.argv) > 1 else find_solver()
    with tempfile.TemporaryDirectory() as tmp:
        test_decoding(tmp)
        test_filter_string()
        test_watchdog(tmp)
        if solver and os.path.exists(PRESETS):
            test_progress_streaming(solver, tmp)
            test_cancel_kills_the_solver(solver, tmp)
        else:
            print("skip solver-backed progress test (no ttm-solver found)")

    print()
    if _failures:
        print(f"{len(_failures)} failed: {', '.join(_failures)}")
        return 1
    print("all worker tests passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
