#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Python wrapper to run parse_test with pipeline JSON config and capture results.

Usage:
    # Use default pipeline (hardcoded in parse_test)
    python3 run_pipeline.py

    # Pass pipeline JSON file
    python3 run_pipeline.py -f pipeline.json

    # Build pipeline config in Python and pass via stdin
    python3 run_pipeline.py --base-file /path/to/log.zip \
                            --inner-dir app/log \
                            --script script_python/exampleGbParse.py \
                            --script2 script_python/script_others.py

    # Pass arbitrary pipeline JSON string
    python3 run_pipeline.py --json '{"nodes":[...],"links":[...]}'
"""

import argparse
import json
import os
import subprocess
import sys
import tempfile


def find_parse_test():
    """Locate parse_test executable relative to this script."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    candidates = [
        os.path.join(project_root, "build", "example", "parse_test"),
        os.path.join(project_root, "build", "rundir", "RelWithDebInfo", "bin", "parse_test"),
    ]
    for path in candidates:
        if os.path.isfile(path) and os.access(path, os.X_OK):
            return path
    return None


def build_pipeline_json(base_file, inner_dir, script_path, script_path_2,
                        wildcard="sv_user.log.*", output_tag="DSVVDCMAPP",
                        capacity="DSVVDCMAPP;DSVTSPConnectSVC",
                        capacity2="DSVLocationAPP"):
    """Build pipeline JSON config dict matching parse_test's make_default_pipeline_json."""
    pipeline = {
        "nodes": [
            {
                "type": "source", "id": "text_file", "name": "test_read",
                "settings": {
                    "base_file": base_file,
                    "inner_dir": inner_dir,
                    "file_name_wildcard": wildcard,
                },
                "active": True,
            },
            {
                "type": "process", "id": "data_dispatch", "name": "test_dispatch",
                "settings": None,
            },
            {
                "type": "process", "id": "script_caller", "name": "test_process",
                "settings": {
                    "script_file_path": script_path,
                    "output_tag": output_tag,
                    "capacity": capacity,
                },
            },
            {
                "type": "process", "id": "script_caller", "name": "test_process_2",
                "settings": {
                    "script_file_path": script_path_2,
                    "capacity": capacity2,
                },
            },
            {
                "type": "output", "id": "xml_output", "name": "test_output",
                "settings": None,
            },
        ],
        "links": [
            {"from": "test_read", "to": "test_dispatch"},
            {"from": "test_dispatch", "to": "test_process"},
            {"from": "test_dispatch", "to": "test_process_2"},
            {"from": "test_process", "to": "test_output"},
            {"from": "test_process_2", "to": "test_output"},
        ],
    }
    return pipeline


def get_env(result_file=None):
    """Build environment with OLS plugin paths and optional result file."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    rundir = os.path.join(project_root, "build", "rundir", "RelWithDebInfo")

    env = os.environ.copy()
    env["OLS_SCRIPT_PATH"] = os.path.join(rundir, "lib", "ols-scripting")
    env["OLS_PLUGINS_PATH"] = os.path.join(rundir, "lib", "ols-plugins")
    env["OLS_PLUGINS_DATA_PATH"] = os.path.join(rundir, "share", "ols")
    if result_file:
        env["OLS_RESULT_FILE"] = result_file
    return env


def run_parse_test(exe, pipeline_json_str=None, json_file=None, env=None):
    """
    Run parse_test and return (returncode, result_dict, stdout, stderr).

    Uses OLS_RESULT_FILE env var to get clean JSON output,
    avoiding stdout pollution from blog() log messages.
    """
    if pipeline_json_str and json_file:
        raise ValueError("Specify only one of pipeline_json_str or json_file")

    # Use a temp file for result output so it doesn't mix with blog() logs
    result_fd, result_path = tempfile.mkstemp(suffix=".json", prefix="ols_result_")
    os.close(result_fd)

    if env is None:
        env = get_env(result_path)
    else:
        env = env.copy()
        env["OLS_RESULT_FILE"] = result_path

    try:
        if json_file:
            cmd = [exe, json_file]
            stdin_data = None
        elif pipeline_json_str:
            cmd = [exe, "-"]
            stdin_data = pipeline_json_str.encode("utf-8")
        else:
            cmd = [exe]
            stdin_data = None

        proc = subprocess.run(
            cmd,
            input=stdin_data,
            capture_output=True,
            env=env,
            timeout=120,
        )

        stdout = proc.stdout.decode("utf-8", errors="replace")
        stderr = proc.stderr.decode("utf-8", errors="replace")

        # Read result JSON from temp file
        result = None
        try:
            with open(result_path, "r") as f:
                result = json.load(f)
        except (json.JSONDecodeError, FileNotFoundError):
            # Fallback: try to find JSON in stdout
            for line in stdout.strip().splitlines():
                line = line.strip()
                if line.startswith("{"):
                    try:
                        result = json.loads(line)
                    except json.JSONDecodeError:
                        continue

        return proc.returncode, result, stdout, stderr

    finally:
        if os.path.exists(result_path):
            os.unlink(result_path)


def main():
    parser = argparse.ArgumentParser(description="Run OLS parse_test pipeline from Python")
    parser.add_argument("-f", "--file", help="Pipeline JSON config file path")
    parser.add_argument("--json", dest="json_str", help="Pipeline JSON string")
    parser.add_argument("--base-file", help="Source base file path (e.g. log.zip)")
    parser.add_argument("--inner-dir", default="", help="Inner directory within archive")
    parser.add_argument("--script", default="script_python/exampleGbParse.py",
                        help="First script path")
    parser.add_argument("--script2", default="script_python/script_others.py",
                        help="Second script path")
    parser.add_argument("--wildcard", default="sv_user.log.*",
                        help="File name wildcard for source")
    parser.add_argument("--exe", help="Path to parse_test executable")

    args = parser.parse_args()

    exe = args.exe or find_parse_test()
    if not exe:
        print("ERROR: Cannot find parse_test executable. Use --exe to specify.", file=sys.stderr)
        sys.exit(1)

    env = get_env()

    if args.file:
        returncode, result, stdout, stderr = run_parse_test(exe, json_file=args.file, env=env)
    elif args.json_str:
        returncode, result, stdout, stderr = run_parse_test(exe, pipeline_json_str=args.json_str, env=env)
    elif args.base_file:
        pipeline = build_pipeline_json(
            base_file=args.base_file,
            inner_dir=args.inner_dir,
            script_path=args.script,
            script_path_2=args.script2,
            wildcard=args.wildcard,
        )
        pipeline_json_str = json.dumps(pipeline, ensure_ascii=False)
        returncode, result, stdout, stderr = run_parse_test(exe, pipeline_json_str=pipeline_json_str, env=env)
    else:
        returncode, result, stdout, stderr = run_parse_test(exe, env=env)

    if returncode != 0:
        print(f"parse_test exited with code {returncode}", file=sys.stderr)

    if result:
        print("=== Pipeline Result ===")
        print(f"Status: {result.get('status', 'unknown')}")
        files = result.get("result_files", [])
        print(f"Generated files ({len(files)}):")
        for f in files:
            print(f"  {f}")
    else:
        print("No result JSON found.", file=sys.stderr)
        print(f"stdout (last 20 lines):", file=sys.stderr)
        for line in stdout.strip().splitlines()[-20:]:
            print(f"  {line}", file=sys.stderr)

    return returncode


if __name__ == "__main__":
    sys.exit(main())
