#!/usr/bin/env python3
"""Report available stable updates for the project's CPM dependency pins."""

import os
import re
import subprocess
import sys
from pathlib import Path


PACKAGE_CALL = re.compile(
    r'^[ \t]*CPMAddPackage\s*\(((?:"[^"\n]*"|#[^\n]*|[^"#)])*)\)',
    re.IGNORECASE | re.MULTILINE,
)
ARGUMENT = re.compile(r'"([^"\n]*)"|#[^\n]*|([^\s"#]+)')
SHORTHAND = re.compile(r"gh:([^#@]+)[#@](.+)")
VERSION_TAG = re.compile(r"(?:v|release[-_])?([0-9]+(?:\.[0-9]+)*)([a-z]?)")


def read_pins(path):
    for call in PACKAGE_CALL.finditer(path.read_text(encoding="utf-8")):
        # Tokenize comments and quoted arguments together so a quoted # survives.
        args = [
            quoted or bare
            for quoted, bare in ARGUMENT.findall(call[1])
            if quoted or bare
        ]
        if len(args) == 1:
            match = SHORTHAND.fullmatch(args[0])
            if match:
                repo, tag = match.groups()
                yield f"https://github.com/{repo}.git", tag
        elif "GIT_REPOSITORY" in args and "GIT_TAG" in args:
            yield args[args.index("GIT_REPOSITORY") + 1], args[args.index("GIT_TAG") + 1]


def version_key(repo, tag):
    match = VERSION_TAG.fullmatch(tag)
    if match is None:
        return None

    number, revision = match.groups()
    version = tuple(map(int, number.split(".")))
    # Dear ImGui uses letter suffixes for stable hotfixes, e.g. v1.92.9b.
    if revision and repo != "ocornut/imgui":
        return None
    # SDL 3 uses odd minor or patch numbers for development releases.
    if repo == "libsdl-org/sdl" and len(version) == 3 and version[0] >= 3:
        if version[1] % 2 or version[2] % 2:
            return None
    return version, revision


def latest_stable_tag(url, repo):
    result = subprocess.run(
        ["git", "ls-remote", "--tags", "--refs", "--", url],
        capture_output=True,
        encoding="utf-8",
        errors="replace",
        env={**os.environ, "GIT_TERMINAL_PROMPT": "0"},
        timeout=25,
    )
    if result.returncode:
        raise RuntimeError(result.stderr.strip() or f"Git exited with status {result.returncode}")

    candidates = []
    for line in result.stdout.splitlines():
        tag = line.partition("\trefs/tags/")[2]
        version = version_key(repo, tag)
        if version is not None:
            candidates.append((version, tag))
    if not candidates:
        raise RuntimeError("no recognizable stable version tags")
    return max(candidates)


def print_table(rows):
    header = ("Dependency", "Pinned tag", "Latest tag", "Status")
    widths = [max(map(len, column)) for column in zip(header, *rows)]
    separator = ["-" * width for width in widths]
    for row in (header, separator, *rows):
        print("  ".join(cell.ljust(width) for cell, width in zip(row, widths)).rstrip())


def main():
    root = Path(__file__).resolve().parent
    try:
        pins = list(read_pins(root / "CMakeLists.txt"))
        pins.extend(read_pins(root / "cmake" / "dependencies.cmake"))
    except OSError as error:
        print(error, file=sys.stderr)
        return 1
    if not pins:
        print("No CPM Git dependencies found.", file=sys.stderr)
        return 1

    rows = []
    errors = []
    exit_code = 0
    for url, current in pins:
        repo = url.removeprefix("https://github.com/").removesuffix(".git")
        current_version = version_key(repo.lower(), current)
        try:
            latest_version, latest_tag = latest_stable_tag(url, repo.lower())
            if current_version is None:
                status = "cannot compare pinned ref"
                exit_code = 1
            elif latest_version > current_version:
                status = "update available"
            else:
                status = "up to date"
            rows.append((repo, current, latest_tag, status))
        except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
            rows.append((repo, current, "-", "check failed"))
            errors.append(f"{repo}: {error}")
            exit_code = 1

    print_table(rows)
    if errors:
        sys.stdout.flush()
        print("\n".join(errors), file=sys.stderr)
    return exit_code


if __name__ == "__main__":
    sys.exit(main())
