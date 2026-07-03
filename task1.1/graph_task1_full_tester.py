#!/usr/bin/env python3
"""
graph_task1_full_tester.py

A stronger black-box tester for Graph Algorithms homework, Task 1.1.
It compiles a C++ solution, runs deterministic cases, checks RPO by invariants,
runs randomized DAG scenarios, and optionally runs sanitizer builds.

Usage:
  python3 graph_task1_full_tester.py --source-dir .
  python3 graph_task1_full_tester.py --binary ./task1.1 --no-build
  python3 graph_task1_full_tester.py --source-dir . --random-tests 300 --sanitize

The tester intentionally normalizes only trailing spaces/newlines. Extra words,
extra lines, wrong names in diagnostics, duplicated RPO nodes and false loops are failures.
"""
from __future__ import annotations

import argparse
import os
import random
import re
import shlex
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Iterable, Optional

CheckResult = tuple[bool, str]
Checker = Callable[[str], CheckResult]

LOOP_RE = re.compile(r"^Found loop\s+(\S+)->(\S+)\s*$")
UNKNOWN_NODE_RE = re.compile(r"^Unknown node\s+(\S+)\s*$")
UNKNOWN_NODES_RE = re.compile(r"^Unknown nodes\s+(\S+)\s+(\S+)\s*$")


@dataclass(frozen=True)
class TestCase:
    name: str
    input_data: str
    check: Checker
    group: str = "deterministic"
    timeout: float = 2.0


@dataclass
class RunResult:
    returncode: int
    stdout: str
    stderr: str
    timed_out: bool = False


@dataclass
class TestFailure:
    name: str
    group: str
    input_data: str
    stdout: str
    stderr: str
    reason: str


class RefGraph:
    def __init__(self) -> None:
        self.nodes: set[str] = set()
        self.out: dict[str, set[str]] = {}
        self.inc: dict[str, set[str]] = {}

    def add_node(self, v: str) -> None:
        self.nodes.add(v)
        self.out.setdefault(v, set())
        self.inc.setdefault(v, set())

    def add_edge(self, u: str, v: str) -> str | None:
        if u not in self.nodes and v not in self.nodes:
            return f"Unknown nodes {u} {v}"
        if u not in self.nodes:
            return f"Unknown node {u}"
        if v not in self.nodes:
            return f"Unknown node {v}"
        self.out.setdefault(u, set()).add(v)
        self.inc.setdefault(v, set()).add(u)
        return None

    def remove_edge(self, u: str, v: str) -> str | None:
        if u not in self.nodes and v not in self.nodes:
            return f"Unknown nodes {u} {v}"
        if u not in self.nodes:
            return f"Unknown node {u}"
        if v not in self.nodes:
            return f"Unknown node {v}"
        self.out.get(u, set()).discard(v)
        self.inc.get(v, set()).discard(u)
        return None

    def remove_node(self, v: str) -> str | None:
        if v not in self.nodes:
            return f"Unknown node {v}"
        for pred in list(self.inc.get(v, set())):
            self.out[pred].discard(v)
        for succ in list(self.out.get(v, set())):
            self.inc[succ].discard(v)
        self.nodes.remove(v)
        self.out.pop(v, None)
        self.inc.pop(v, None)
        return None

    def reachable(self, start: str) -> set[str]:
        if start not in self.nodes:
            return set()
        seen: set[str] = set()
        stack = [start]
        while stack:
            u = stack.pop()
            if u in seen:
                continue
            seen.add(u)
            stack.extend(self.out.get(u, set()) - seen)
        return seen

    def reachable_edges(self, start: str) -> set[tuple[str, str]]:
        reach = self.reachable(start)
        return {(u, v) for u in reach for v in self.out.get(u, set()) if v in reach}


def strip_blank_edges(text: str) -> str:
    return text.strip("\n")


def normalized_lines(text: str) -> list[str]:
    if text.strip() == "":
        return []
    return [line.rstrip() for line in strip_blank_edges(text).splitlines()]


def visible(text: str) -> str:
    if text == "":
        return "<empty>"
    return text.rstrip("\n")


def exact(expected: str) -> Checker:
    expected_lines = normalized_lines(expected)

    def check(actual: str) -> CheckResult:
        actual_lines = normalized_lines(actual)
        if actual_lines == expected_lines:
            return True, ""
        return False, f"exact mismatch\nexpected: {expected_lines}\nactual:   {actual_lines}"

    return check


def no_output() -> Checker:
    def check(actual: str) -> CheckResult:
        if actual.strip() == "":
            return True, ""
        return False, f"expected no stdout, got: {normalized_lines(actual)}"

    return check


def split_rpo_output(output: str) -> tuple[list[tuple[str, str, str]], list[str], list[str]]:
    """Return (loop_lines as (raw,u,v), non_loop_lines, node_tokens)."""
    loops: list[tuple[str, str, str]] = []
    non_loop_lines: list[str] = []
    tokens: list[str] = []
    for line in normalized_lines(output):
        m = LOOP_RE.match(line)
        if m:
            loops.append((line, m.group(1), m.group(2)))
        else:
            non_loop_lines.append(line)
            tokens.extend(line.split())
    return loops, non_loop_lines, tokens


def check_rpo_invariant(
    *,
    expected_nodes: set[str],
    edges_must_go_forward: Iterable[tuple[str, str]] = (),
    exact_loops: Optional[set[str]] = None,
    min_loop_count: Optional[int] = None,
    allow_loops: bool = False,
) -> Checker:
    required_edges = set(edges_must_go_forward)

    def check(actual: str) -> CheckResult:
        loops, non_loop_lines, tokens = split_rpo_output(actual)
        loop_lines = {raw for raw, _, _ in loops}

        if exact_loops is not None and loop_lines != exact_loops:
            return False, f"loop lines mismatch\nexpected: {sorted(exact_loops)}\nactual:   {sorted(loop_lines)}"
        if not allow_loops and exact_loops is None and loops:
            return False, f"unexpected loop diagnostics: {sorted(loop_lines)}"
        if min_loop_count is not None and len(loops) < min_loop_count:
            return False, f"expected at least {min_loop_count} loop diagnostic(s), got {sorted(loop_lines)}"

        if set(tokens) != expected_nodes:
            return False, f"wrong RPO node set\nexpected: {sorted(expected_nodes)}\nactual tokens: {tokens}\nnon-loop lines: {non_loop_lines}"
        if len(tokens) != len(set(tokens)):
            return False, f"duplicate node(s) in RPO output: {tokens}"

        pos = {node: i for i, node in enumerate(tokens)}
        for u, v in sorted(required_edges):
            if u not in pos or v not in pos:
                return False, f"edge {u}->{v} cannot be checked because output is {tokens}"
            if pos[u] >= pos[v]:
                return False, f"RPO/topological invariant failed for edge {u}->{v}; order: {tokens}"

        return True, ""

    return check


def check_combined(expected_prefix: str, rpo_checker: Checker) -> Checker:
    """Check exact diagnostic prefix, then run RPO checker on the rest."""
    prefix_lines = normalized_lines(expected_prefix)

    def check(actual: str) -> CheckResult:
        lines = normalized_lines(actual)
        if lines[: len(prefix_lines)] != prefix_lines:
            return False, f"prefix mismatch\nexpected prefix: {prefix_lines}\nactual lines:    {lines}"
        rest = "\n".join(lines[len(prefix_lines) :])
        return rpo_checker(rest)

    return check


def make_deterministic_tests() -> list[TestCase]:
    t: list[TestCase] = []

    t.append(TestCase(
        "successful NODE commands are silent",
        """
NODE A
NODE B
NODE C
""",
        no_output(),
    ))

    t.append(TestCase(
        "EDGE reports unknown endpoints and real names",
        """
EDGE A B 5
NODE X
EDGE X Y 10
EDGE U X 1
""",
        exact("""
Unknown nodes A B
Unknown node Y
Unknown node U
"""),
    ))

    t.append(TestCase(
        "successful EDGE is silent",
        """
NODE A
NODE B
EDGE A B 5
""",
        no_output(),
    ))

    t.append(TestCase(
        "REMOVE NODE reports unknown node and success is silent",
        """
REMOVE NODE A
NODE A
REMOVE NODE A
""",
        exact("""
Unknown node A
"""),
    ))

    t.append(TestCase(
        "REMOVE EDGE reports unknown endpoints and real names",
        """
REMOVE EDGE A B
NODE A
REMOVE EDGE A B
REMOVE EDGE B A
NODE B
REMOVE EDGE A B
""",
        exact("""
Unknown nodes A B
Unknown node B
Unknown node B
"""),
    ))

    t.append(TestCase(
        "simple RPO chain exact",
        """
NODE A
NODE B
NODE C
EDGE A B 1
EDGE B C 1
RPO_NUMBERING A
""",
        exact("A B C\n"),
    ))

    t.append(TestCase(
        "RPO diamond DAG has no false loop and no duplicate D",
        """
NODE A
NODE B
NODE C
NODE D
EDGE A B 1
EDGE A C 1
EDGE B D 1
EDGE C D 1
RPO_NUMBERING A
""",
        check_rpo_invariant(
            expected_nodes={"A", "B", "C", "D"},
            edges_must_go_forward={("A", "B"), ("A", "C"), ("B", "D"), ("C", "D")},
        ),
    ))

    t.append(TestCase(
        "RPO ignores unreachable node from start",
        """
NODE A
NODE B
NODE Z
EDGE A B 1
RPO_NUMBERING A
""",
        check_rpo_invariant(
            expected_nodes={"A", "B"},
            edges_must_go_forward={("A", "B")},
        ),
    ))

    t.append(TestCase(
        "cycle A->B->C->A is reported before sequence",
        """
NODE A
NODE B
NODE C
EDGE A B 1
EDGE B C 1
EDGE C A 1
RPO_NUMBERING A
""",
        check_rpo_invariant(
            expected_nodes={"A", "B", "C"},
            exact_loops={"Found loop C->A"},
            allow_loops=True,
        ),
    ))

    t.append(TestCase(
        "self-loop is reported and printed once",
        """
NODE A
EDGE A A 1
RPO_NUMBERING A
""",
        check_rpo_invariant(
            expected_nodes={"A"},
            exact_loops={"Found loop A->A"},
            allow_loops=True,
        ),
    ))

    t.append(TestCase(
        "REMOVE EDGE detaches adjacency",
        """
NODE A
NODE B
NODE C
EDGE A B 1
EDGE B C 1
REMOVE EDGE B C
RPO_NUMBERING A
""",
        check_rpo_invariant(
            expected_nodes={"A", "B"},
            edges_must_go_forward={("A", "B")},
        ),
    ))

    t.append(TestCase(
        "REMOVE EDGE of absent edge is silent but graph remains valid",
        """
NODE A
NODE B
NODE C
EDGE A B 1
REMOVE EDGE B C
RPO_NUMBERING A
""",
        check_rpo_invariant(
            expected_nodes={"A", "B"},
            edges_must_go_forward={("A", "B")},
        ),
    ))

    t.append(TestCase(
        "REMOVE NODE detaches incoming and outgoing edges",
        """
NODE A
NODE B
NODE C
NODE D
EDGE A B 1
EDGE B C 1
EDGE D B 1
REMOVE NODE B
RPO_NUMBERING A
RPO_NUMBERING D
""",
        exact("""
A
D
"""),
    ))

    t.append(TestCase(
        "REMOVE NODE with self-loop does not leave dangling edge",
        """
NODE A
EDGE A A 1
REMOVE NODE A
NODE A
RPO_NUMBERING A
""",
        exact("A\n"),
        timeout=2.0,
    ))

    t.append(TestCase(
        "REMOVE NODE inside a cycle cleans all incident edges",
        """
NODE A
NODE B
NODE C
EDGE A B 1
EDGE B C 1
EDGE C A 1
REMOVE NODE B
RPO_NUMBERING A
RPO_NUMBERING C
""",
        exact("""
A
C A
"""),
    ))

    t.append(TestCase(
        "RPO can be called twice after mutations",
        """
NODE A
NODE B
NODE C
EDGE A B 1
RPO_NUMBERING A
EDGE B C 1
RPO_NUMBERING A
REMOVE EDGE A B
RPO_NUMBERING A
""",
        exact("""
A B
A B C
A
"""),
    ))

    t.append(TestCase(
        "large chain recursion/basic performance",
        "\n".join([*(f"NODE N{i}" for i in range(200)), *(f"EDGE N{i} N{i+1} 1" for i in range(199)), "RPO_NUMBERING N0", ""]),
        check_rpo_invariant(
            expected_nodes={f"N{i}" for i in range(200)},
            edges_must_go_forward={(f"N{i}", f"N{i+1}") for i in range(199)},
        ),
        group="stress",
        timeout=5.0,
    ))

    return t


def generate_random_dag_case(seed: int, n: int, p: float, remove_fraction: float) -> TestCase:
    rng = random.Random(seed)
    g = RefGraph()
    cmds: list[str] = []

    names = [f"N{i}" for i in range(n)]
    for name in names:
        cmds.append(f"NODE {name}")
        g.add_node(name)

    edges: list[tuple[str, str]] = []
    for i in range(n):
        for j in range(i + 1, n):
            if rng.random() < p:
                u, v = names[i], names[j]
                cmds.append(f"EDGE {u} {v} {rng.randint(0, 100)}")
                g.add_edge(u, v)
                edges.append((u, v))

    rng.shuffle(edges)
    for u, v in edges[: int(len(edges) * remove_fraction)]:
        cmds.append(f"REMOVE EDGE {u} {v}")
        g.remove_edge(u, v)

    # Remove a few non-start nodes to exercise incident-edge cleanup.
    start = names[0]
    removable = names[1:]
    rng.shuffle(removable)
    for v in removable[: max(0, n // 10)]:
        if rng.random() < 0.55:
            cmds.append(f"REMOVE NODE {v}")
            g.remove_node(v)

    if start not in g.nodes:
        cmds.append(f"NODE {start}")
        g.add_node(start)

    cmds.append(f"RPO_NUMBERING {start}")
    expected_nodes = g.reachable(start)
    expected_edges = g.reachable_edges(start)

    return TestCase(
        name=f"random DAG seed={seed} n={n} p={p:.2f} remove={remove_fraction:.2f}",
        input_data="\n".join(cmds) + "\n",
        check=check_rpo_invariant(expected_nodes=expected_nodes, edges_must_go_forward=expected_edges),
        group="random",
        timeout=5.0,
    )


def make_random_tests(count: int, seed: int) -> list[TestCase]:
    rng = random.Random(seed)
    tests: list[TestCase] = []
    for i in range(count):
        n = rng.randint(2, 35)
        p = rng.uniform(0.03, 0.22)
        remove_fraction = rng.choice([0.0, 0.1, 0.25, 0.4])
        tests.append(generate_random_dag_case(seed=rng.randrange(1 << 30), n=n, p=p, remove_fraction=remove_fraction))
    return tests


def make_robustness_tests() -> list[TestCase]:
    """Tests outside the official guarantee. Disabled unless --include-robustness is passed."""
    return [
        TestCase(
            "duplicate NODE must not corrupt existing edges",
            """
NODE A
NODE B
EDGE A B 1
NODE A
RPO_NUMBERING A
""",
            check_rpo_invariant(expected_nodes={"A", "B"}, edges_must_go_forward={("A", "B")}),
            group="robustness",
        ),
        TestCase(
            "duplicate EDGE must not leave dangling old edge pointers",
            """
NODE A
NODE B
EDGE A B 1
EDGE A B 2
REMOVE EDGE A B
RPO_NUMBERING A
""",
            exact("A\n"),
            group="robustness",
        ),
        TestCase(
            "reverse edges cycle is detected if input violates guarantee",
            """
NODE A
NODE B
EDGE A B 1
EDGE B A 1
RPO_NUMBERING A
""",
            check_rpo_invariant(expected_nodes={"A", "B"}, min_loop_count=1, allow_loops=True),
            group="robustness",
        ),
    ]


def find_cpp_files(source_dir: Path) -> list[Path]:
    ignored_parts = {"cmake-build-debug", "cmake-build-release", "build", ".git", ".idea", "out"}
    files: list[Path] = []
    for p in source_dir.rglob("*.cpp"):
        rel_parts = set(p.relative_to(source_dir).parts)
        if rel_parts & ignored_parts:
            continue
        files.append(p)
    return sorted(files)


def run_cmd(cmd: list[str], *, cwd: Path, input_data: str | None = None, timeout: float = 20.0, env: Optional[dict[str, str]] = None) -> RunResult:
    try:
        completed = subprocess.run(
            cmd,
            cwd=cwd,
            input=input_data,
            text=True,
            capture_output=True,
            timeout=timeout,
            env=env,
        )
        return RunResult(completed.returncode, completed.stdout, completed.stderr, False)
    except subprocess.TimeoutExpired as e:
        return RunResult(-999, e.stdout or "", e.stderr or "", True)


def compile_direct(source_dir: Path, exe: Path, compiler: str, extra_flags: list[str], sanitize: bool) -> tuple[bool, str]:
    cpp_files = find_cpp_files(source_dir)
    if not cpp_files:
        return False, f"no .cpp files found under {source_dir}"

    flags = ["-std=c++23", "-Wall", "-Wextra", "-pedantic", "-g"]
    if sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    flags += extra_flags

    cmd = [compiler, *flags, *map(str, cpp_files), "-o", str(exe)]
    res = run_cmd(cmd, cwd=source_dir, timeout=40.0)
    if res.returncode != 0:
        return False, "$ " + " ".join(shlex.quote(x) for x in cmd) + "\n" + res.stdout + res.stderr
    return True, "$ " + " ".join(shlex.quote(x) for x in cmd)


def compile_with_command(source_dir: Path, build_command: str) -> tuple[bool, str]:
    res = subprocess.run(build_command, shell=True, cwd=source_dir, text=True, capture_output=True, timeout=60.0)
    if res.returncode != 0:
        return False, f"$ {build_command}\n{res.stdout}{res.stderr}"
    return True, f"$ {build_command}\n{res.stdout}{res.stderr}"


def run_program(exe: Path, input_data: str, timeout: float, sanitize: bool = False) -> RunResult:
    env = os.environ.copy()
    if sanitize:
        # Make sanitizer failures loud and deterministic.
        env.setdefault("ASAN_OPTIONS", "detect_leaks=1:abort_on_error=1:halt_on_error=1")
        env.setdefault("UBSAN_OPTIONS", "abort_on_error=1:halt_on_error=1:print_stacktrace=1")
    return run_cmd([str(exe)], cwd=exe.parent, input_data=input_data.strip() + "\n", timeout=timeout, env=env)


def run_tests(exe: Path, tests: list[TestCase], *, fail_fast: bool, sanitize_runtime: bool) -> tuple[int, list[TestFailure]]:
    passed = 0
    failures: list[TestFailure] = []
    for idx, test in enumerate(tests, start=1):
        res = run_program(exe, test.input_data, test.timeout, sanitize=sanitize_runtime)
        if res.timed_out:
            ok, reason = False, f"timeout after {test.timeout:.1f}s"
        elif res.returncode != 0:
            ok, reason = False, f"non-zero exit code {res.returncode}"
            if res.stderr.strip():
                reason += "\nstderr:\n" + res.stderr.rstrip()
        else:
            ok, reason = test.check(res.stdout)

        if ok:
            print(f"[PASS] {idx:03d} {test.group:13s} {test.name}")
            passed += 1
        else:
            print(f"[FAIL] {idx:03d} {test.group:13s} {test.name}")
            failures.append(TestFailure(test.name, test.group, test.input_data, res.stdout, res.stderr, reason))
            if fail_fast:
                break
    return passed, failures


def print_failures(failures: list[TestFailure], limit: int) -> None:
    if not failures:
        return
    print("\n=== FAILURES ===")
    for i, f in enumerate(failures[:limit], start=1):
        print(f"\n--- failure #{i}: [{f.group}] {f.name} ---")
        print("input:")
        print(f.input_data.strip())
        print("stdout:")
        print(visible(f.stdout))
        if f.stderr.strip():
            print("stderr:")
            print(f.stderr.rstrip())
        print("reason:")
        print(f.reason)
    if len(failures) > limit:
        print(f"\n... {len(failures) - limit} more failure(s) omitted. Increase --show-failures.")


def static_scan(source_dir: Path, strict_static: bool) -> tuple[bool, list[str]]:
    warnings: list[str] = []
    files = {p.name: p for p in source_dir.rglob("*") if p.is_file() and p.suffix in {".h", ".hpp", ".cpp"}}

    required = ["graph.hpp", "vertex.hpp", "edge.hpp"]
    for name in required:
        if name not in files:
            warnings.append(f"missing expected source file {name}")

    edge_h = files.get("edge.hpp")
    if edge_h:
        text = edge_h.read_text(encoding="utf-8", errors="ignore")
        has_vertex_pointer = bool(re.search(r"\bVertex\s*\*", text))
        has_strings = "std::string" in text or "string" in text
        if not has_vertex_pointer:
            warnings.append("edge.hpp: Edge does not visibly store Vertex* predecessor/successor; assignment requires pointers to nodes")
        if has_strings and not has_vertex_pointer:
            warnings.append("edge.hpp: Edge appears to store string ids instead of node pointers")

    graph_h = files.get("graph.hpp")
    if graph_h:
        text = graph_h.read_text(encoding="utf-8", errors="ignore")
        if "std::optional" in text and "#include <optional>" not in text:
            warnings.append("graph.hpp uses std::optional but does not explicitly include <optional>")
        if "std::reference_wrapper" in text and "#include <functional>" not in text:
            warnings.append("graph.hpp uses std::reference_wrapper but does not explicitly include <functional>")
        if "std::unordered_set" in text and "#include <unordered_set>" not in text:
            warnings.append("graph.hpp uses std::unordered_set but does not explicitly include <unordered_set>")
        if re.search(r"\w+_\s*\[\s*name\s*\]", text):
            warnings.append("graph.hpp contains map[name]-style access; check it does not create missing vertices in getters/removers")
        if "SrcAndDestUnknown = SrcUnknown | DestUnknown" in text:
            warnings.append("graph.hpp: enum class value uses operator| inside enum; portable C++ requires explicit value 6 or predeclared operator support")

    main_cpp = files.get("main.cpp")
    if main_cpp:
        text = main_cpp.read_text(encoding="utf-8", errors="ignore")
        if "Unknown node A" in text or "Unknown nodes A B" in text:
            warnings.append("main.cpp contains literal Unknown node A/B messages; diagnostics should print actual command arguments")
        if "std::unordered_map" in text and "#include <unordered_map>" not in text:
            warnings.append("main.cpp uses std::unordered_map but does not explicitly include <unordered_map>")
        if "std::reverse" in text and "#include <algorithm>" not in text:
            warnings.append("main.cpp uses std::reverse but does not explicitly include <algorithm>")

    ok = not warnings or not strict_static
    return ok, warnings


def main() -> int:
    parser = argparse.ArgumentParser(description="Full tester for graph homework Task 1.1")
    parser.add_argument("--source-dir", default=".", help="directory with C++ sources; default: current dir")
    parser.add_argument("--binary", help="existing executable to test; implies --no-build unless --build-command is also passed")
    parser.add_argument("--compiler", default=os.environ.get("CXX", "g++"), help="C++ compiler for direct build")
    parser.add_argument("--exe-name", default=".graph_task1_test_bin", help="temporary executable name for normal build")
    parser.add_argument("--build-command", help="custom shell command to build the solution")
    parser.add_argument("--no-build", action="store_true", help="do not compile; use --binary")
    parser.add_argument("--extra-flag", action="append", default=[], help="extra compiler flag; may be repeated")
    parser.add_argument("--sanitize", action="store_true", help="also build and run deterministic/stress tests with ASan+UBSan")
    parser.add_argument("--strict-static", action="store_true", help="treat static source warnings as failures")
    parser.add_argument("--random-tests", type=int, default=100, help="number of randomized DAG tests; default: 100")
    parser.add_argument("--seed", type=int, default=20260702, help="random seed for generated tests")
    parser.add_argument("--include-robustness", action="store_true", help="also test duplicate/reverse-edge cases outside official guarantees")
    parser.add_argument("--fail-fast", action="store_true", help="stop after the first failed test")
    parser.add_argument("--show-failures", type=int, default=8, help="how many failure details to print")
    args = parser.parse_args()

    source_dir = Path(args.source_dir).resolve()
    if not source_dir.exists():
        print(f"[FATAL] source directory does not exist: {source_dir}")
        return 2

    print(f"source-dir: {source_dir}")
    static_ok, static_warnings = static_scan(source_dir, args.strict_static)
    if static_warnings:
        print("\n=== STATIC SCAN ===")
        for w in static_warnings:
            prefix = "[STATIC-FAIL]" if args.strict_static else "[STATIC-WARN]"
            print(f"{prefix} {w}")
    if not static_ok:
        print("\nStatic scan failed because --strict-static was enabled.")
        return 1

    if args.binary:
        exe = Path(args.binary).resolve()
    else:
        exe = source_dir / args.exe_name

    if args.no_build and not args.binary:
        print("[FATAL] --no-build requires --binary")
        return 2

    if not args.no_build:
        print("\n=== BUILD ===")
        if args.build_command:
            ok, build_log = compile_with_command(source_dir, args.build_command)
        else:
            ok, build_log = compile_direct(source_dir, exe, args.compiler, args.extra_flag, sanitize=False)
        print(build_log.rstrip())
        if not ok:
            print("[BUILD-FAIL]")
            return 1
        if not exe.exists() and args.binary:
            print(f"[FATAL] build succeeded but binary does not exist: {exe}")
            return 2

    if not exe.exists():
        print(f"[FATAL] executable does not exist: {exe}")
        return 2

    tests = make_deterministic_tests() + make_random_tests(args.random_tests, args.seed)
    if args.include_robustness:
        tests += make_robustness_tests()

    print("\n=== RUNTIME TESTS ===")
    passed, failures = run_tests(exe, tests, fail_fast=args.fail_fast, sanitize_runtime=False)
    print(f"\nnormal result: {passed}/{len(tests)} passed")
    print_failures(failures, args.show_failures)

    sanitizer_failed = False
    if args.sanitize and not args.no_build:
        print("\n=== SANITIZER BUILD/RUN ===")
        sanitize_exe = source_dir / ".graph_task1_asan_ubsan_bin"
        ok, build_log = compile_direct(source_dir, sanitize_exe, args.compiler, args.extra_flag, sanitize=True)
        print(build_log.rstrip())
        if not ok:
            print("[SANITIZER-BUILD-FAIL]")
            sanitizer_failed = True
        else:
            # Sanitizer run covers deterministic + stress; random already covered normally.
            san_tests = [test for test in make_deterministic_tests() if test.group in {"deterministic", "stress"}]
            san_passed, san_failures = run_tests(sanitize_exe, san_tests, fail_fast=args.fail_fast, sanitize_runtime=True)
            print(f"\nsan result: {san_passed}/{len(san_tests)} passed")
            print_failures(san_failures, args.show_failures)
            sanitizer_failed = bool(san_failures)

    if failures or sanitizer_failed:
        print("\nVERDICT: FAIL")
        return 1

    print("\nVERDICT: PASS")
    if static_warnings and not args.strict_static:
        print("Note: runtime passed, but static warnings remain. Use --strict-static to make them fatal.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
