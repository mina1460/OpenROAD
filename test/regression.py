#!/usr/bin/env python3
import subprocess
import sys
import typing as t
import concurrent.futures
import colorama
from colorama import Fore
import os

colorama.init(autoreset=True)

tool_scripts = [
    "ant",
    "dbSta",
    "dft",
    "dpl",
    "drt",
    "fin",
    "gpl",
    "cts",
    "grt",
    "gui",
    "ifp",
    "mpl",
    "dpo",
    "mpl2",
    "OpenDB",
    "pad",
    "par",
    "pdn",
    "ppl",
    "psm",
    "rcx",
    "rmp",
    "rsz",
    "stt",
    "tap",
    "upf",
    "utl",
]


def run_tool_tests(tool_name: str) -> int:
    try:
        result = subprocess.run(
            ["/bin/bash", "./test/regression_helper", tool_name],
            capture_output=True,
            text=True,
        )
        tests = result.stdout.splitlines()
        tool_tests_failures: int = 0
        for test in tests:
            if ") pass" not in test and any(
                test_method in test for test_method in ["(py)", "(tcl)"]
            ):
                tool_tests_failures += 1
        return tool_tests_failures
    except subprocess.CalledProcessError:
        return 1


def clear_screen():
    if os.name == "posix":
        os.system("clear")
    elif os.name == "nt":
        os.system("cls")


def run_tests_with_progress(tool_name: str):
    try:
        failures = run_tool_tests(tool_name)
        return {"tool": tool_name, "failures": failures}
    except Exception as e:
        return {"tool": tool_name, "error": str(e)}


if __name__ == "__main__":
    print("Starting...")
    failures: int = 0
    completed_tools_with_failures: t.Dict[str, int] = {}
    completed_tools_no_failures: t.List[str] = []
    waiting_tools = set(tool_scripts)
    failed_tools = []

    if len(sys.argv) != 2:
        print("Usage: python regression.py <number_of_threads>")
        sys.exit(1)

    try:
        num_threads = int(sys.argv[1])
    except ValueError:
        print("Error: Please provide a valid integer for the number of threads.")
        sys.exit(1)

    with concurrent.futures.ThreadPoolExecutor(max_workers=num_threads) as executor:
        futures = {
            executor.submit(run_tests_with_progress, tool): tool
            for tool in tool_scripts
        }

        for future in concurrent.futures.as_completed(futures):
            tool_info = future.result()
            tool_name = str(tool_info["tool"])
            if "failures" in tool_info:
                if int(tool_info["failures"]) > 0:
                    completed_tools_with_failures[tool_name] = int(
                        tool_info["failures"]
                    )
                else:
                    completed_tools_no_failures.append(tool_name)
            elif "error" in tool_info:
                failed_tools.append(tool_name)
            waiting_tools.remove(tool_name)

            clear_screen()
            print(
                f"\nCompleted Jobs: {len(completed_tools_no_failures) + len(completed_tools_with_failures)}/{len(tool_scripts)}"
            )

            print("Successfully: " + Fore.GREEN + str(completed_tools_no_failures))
            print("Failed: " + Fore.RED + str(completed_tools_with_failures))

            print("\nWaiting Jobs:")
            print(Fore.YELLOW + str(waiting_tools))

            # Print any jobs that encountered errors
            if failed_tools:
                print(Fore.RED + "\nJobs with Errors:")
                for tool in failed_tools:
                    print(Fore.RED + f"- {tool}")

        # Wait for all tasks to complete
        concurrent.futures.wait(futures)
        sys.exit(len(failed_tools) + len(completed_tools_with_failures))
