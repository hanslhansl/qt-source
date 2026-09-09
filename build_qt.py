#!/usr/bin/env python3
import os
import shutil
import subprocess
import sys
from pathlib import Path
import traceback
import json
import argparse

# ============================================================
# Global configuration
# ============================================================

SCRIPT_DIR = Path(__file__).resolve().parent

def parse_args():
    parser = argparse.ArgumentParser(
        description="Build Qt using CMake/Ninja."
    )

    parser.add_argument(
        "--additional-path",
        action="append",
        type=Path,
        default=[],
        metavar="PATH",
        help="Add a directory to PATH. Can be specified multiple times.",
    )

    parser.add_argument(
        "--build-dir",
        type=Path,
        default=SCRIPT_DIR / "build",
        metavar="PATH",
        help="Build directory (default: %(default)s)",
    )

    parser.add_argument(
        "--output-dir",
        type=Path,
        default=SCRIPT_DIR / "out",
        metavar="PATH",
        help="Output directory (default: %(default)s)",
    )

    parser.add_argument(
        "--qt-source",
        type=Path,
        default=SCRIPT_DIR / "qt",
        metavar="PATH",
        help="Qt source directory (default: %(default)s)",
    )

    parser.add_argument(
        "--qt-namespace",
        default=None,
        metavar="NAME",
        help="Qt namespace (default: %(default)s)",
    )

    parser.add_argument(
        "--cmake",
        type=Path,
        default=None,
        metavar="PATH",
        help="CMake executable. If omitted, use PATH.",
    )

    parser.add_argument(
        "--ninja",
        type=Path,
        default=None,
        metavar="PATH",
        help="Ninja executable. If omitted, use PATH.",
    )

    parser.add_argument(
        "--qt-submodules",
        default="qtbase,qttools",
        metavar="MODULES",
        help="Comma-separated Qt submodules (default: %(default)s)",
    )

    return parser.parse_args()


SYSTEM_PATHS = [
    Path(os.environ["SystemRoot"]) / "System32",
]

# ============================================================
# Helper functions
# ============================================================

def run_command(command : list[str], cwd=None, env=None):
    """Run a command and stop on failure."""
    print("\nRunning:")
    print(" ".join(str(c) for c in command))
    print()

    result = subprocess.run(
        command,
        cwd=cwd,
        env=env,
    )

    if result.returncode != 0:
        raise RuntimeError(
            f"Command failed with exit code {result.returncode}"
        )

def request_toolchain_info(build_dir: Path):
    query_dir = build_dir / ".cmake" / "api" / "v1" / "query" / "client-build-script"
    query_dir.mkdir(parents=True, exist_ok=True)
    (query_dir / "toolchains-v1").touch()
def get_toolchain_info(build_dir: Path):
    reply_dir = build_dir / ".cmake" / "api" / "v1" / "reply"

    indexes = sorted(reply_dir.glob("index-*.json"))
    if not indexes:
        raise RuntimeError("No CMake File API reply index found")

    index = json.loads(indexes[-1].read_text())

    ref = index["reply"]["client-build-script"]["toolchains-v1"]
    toolchains_file = reply_dir / ref["jsonFile"]

    return json.loads(toolchains_file.read_text())
def get_cxx_compiler(build_dir: Path):
    info = get_toolchain_info(build_dir)

    for toolchain in info["toolchains"]:
        if toolchain["language"] == "CXX":
            return toolchain["compiler"]

    raise RuntimeError("No CXX toolchain found")

def find_tool(tool):
    """Find a tool in the current global PATH."""
    result = shutil.which(tool)
    if result is None:
        raise RuntimeError(f"{tool} not found in PATH")
    return result

def create_build_environment(additional_paths, cmake_bin, ninja_bin):
    env = os.environ.copy()

    path_entries = additional_paths.copy()

    # Add explicit CMake/Ninja locations if configured
    if cmake_bin:
        path_entries.append(cmake_bin)
    else:
        cmake_path = find_tool("cmake")
        path_entries.append(os.path.dirname(cmake_path))

    if ninja_bin:
        path_entries.append(ninja_bin)
    else:
        ninja_path = find_tool("ninja")
        path_entries.append(os.path.dirname(ninja_path))

    # Add Windows system tools
    path_entries.extend(SYSTEM_PATHS)

    env["PATH"] = ";".join(str(p) for p in path_entries)

    return env

# ============================================================
# Main build process
# ============================================================

def main():
    args = parse_args()
    additional_paths = args.additional_path
    build_dir = args.build_dir
    output_dir = args.output_dir
    qt_source = args.qt_source
    qt_namespace = args.qt_namespace
    cmake_bin = args.cmake
    ninja_bin = args.ninja
    qt_submodules = args.qt_submodules


    # ----------------------------------------------------
    # Prepare PATH
    # ----------------------------------------------------
    env = create_build_environment(additional_paths, cmake_bin, ninja_bin)

    # ----------------------------------------------------
    # Set up directories
    # ----------------------------------------------------
    build_dir.mkdir( parents=True, exist_ok=True)
    TEMP_INSTALL_DIR = output_dir / "temp"

    # ----------------------------------------------------
    # CMake file api request for toolchain info
    # ----------------------------------------------------
    request_toolchain_info(build_dir)

    # ----------------------------------------------------
    # Configure Qt
    # ----------------------------------------------------
    configure_cmd = [
        qt_source / "configure.bat",

        "-prefix", TEMP_INSTALL_DIR,

        "-release",
        "-shared", "-force-debug-info", "-separate-debug-info", # shared build
        "-opensource",
        "-confirm-license",

        "-cmake-generator", "Ninja",

        "-submodules", qt_submodules,

        "--",

        "-DQT_INSTALL_CONFIG_INFO_FILES=ON",

        "" if qt_namespace is None else f"-DQT_NAMESPACE={qt_namespace}",
    ]
    run_command(configure_cmd, cwd=build_dir, env=env)

    # ----------------------------------------------------
    # Read the compiler CMake selected
    # ----------------------------------------------------
    compiler = get_cxx_compiler(build_dir)
    compiler_id = compiler["id"]
    compiler_version = compiler["version"]
    toolchain_name = f"{compiler_id.lower()}-{compiler_version.split('.')[0]}"

    # ----------------------------------------------------
    # Build
    # ----------------------------------------------------
    run_command(
        [
            "cmake",
            "--build",
            ".",
            "--parallel"
        ],
        cwd=build_dir,
        env=env
    )

    # ----------------------------------------------------
    # Install
    # ----------------------------------------------------
    run_command(
        [
            "cmake",
            "--install",
            "."
        ],
        cwd=build_dir,
        env=env
    )

    # ----------------------------------------------------
    # Copy to actual install directory
    # ----------------------------------------------------
    FINAL_INSTALL_DIR = output_dir / toolchain_name
    if FINAL_INSTALL_DIR.exists():
        shutil.rmtree(FINAL_INSTALL_DIR)
    TEMP_INSTALL_DIR.rename(FINAL_INSTALL_DIR)

    print(f"\nBuild completed successfully in {FINAL_INSTALL_DIR}!")




if __name__ == "__main__":
    main()