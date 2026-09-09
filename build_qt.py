#!/usr/bin/env python3
import os
import shutil
import subprocess
import sys
from pathlib import Path
import traceback
import json

# ============================================================
# Global configuration
# ============================================================

# e.g. compiler bin
ADDITIONAL_PATHS = [
    Path(r"...\bin")
]
BUILD_DIR = Path(__file__).resolve().parent / "build"
OUTPUT_DIR = Path(__file__).resolve().parent / "out"
QT_SOURCE = Path(__file__).resolve().parent / "qt"

QT_NAMESPACE = None

CMAKE_BIN = None    # None: get from PATH
NINJA_BIN = None    # None: get from PATH

# Qt configure options
QT_SUBMODULES = "qtbase,qttools"

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

def create_build_environment():
    env = os.environ.copy()

    path_entries = ADDITIONAL_PATHS.copy()

    # Add explicit CMake/Ninja locations if configured
    if CMAKE_BIN:
        path_entries.append(CMAKE_BIN)
    else:
        cmake_path = find_tool("cmake")
        path_entries.append(os.path.dirname(cmake_path))

    if NINJA_BIN:
        path_entries.append(NINJA_BIN)
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
    try:
        # ----------------------------------------------------
        # Prepare PATH
        # ----------------------------------------------------
        env = create_build_environment()

        # ----------------------------------------------------
        # Set up directories
        # ----------------------------------------------------
        BUILD_DIR.mkdir( parents=True, exist_ok=True)
        TEMP_INSTALL_DIR = OUTPUT_DIR / "temp"

        # ----------------------------------------------------
        # CMake file api request for toolchain info
        # ----------------------------------------------------
        request_toolchain_info(BUILD_DIR)

        # ----------------------------------------------------
        # Configure Qt
        # ----------------------------------------------------
        configure_cmd = [
            QT_SOURCE / "configure.bat",

            "-prefix", TEMP_INSTALL_DIR,

            "-release",
            "-shared", "-force-debug-info", "-separate-debug-info", # shared build
            "-opensource",
            "-confirm-license",

            "-cmake-generator", "Ninja",

            "-submodules", QT_SUBMODULES,

            "--",

            "-DQT_INSTALL_CONFIG_INFO_FILES=ON",

            "" if QT_NAMESPACE is None else f"-DQT_NAMESPACE={QT_NAMESPACE}",
        ]
        run_command(configure_cmd, cwd=BUILD_DIR, env=env)

        # ----------------------------------------------------
        # Read the compiler CMake selected
        # ----------------------------------------------------
        compiler = get_cxx_compiler(BUILD_DIR)
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
            cwd=BUILD_DIR,
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
            cwd=BUILD_DIR,
            env=env
        )

        # ----------------------------------------------------
        # Copy to actual install directory
        # ----------------------------------------------------
        FINAL_INSTALL_DIR = OUTPUT_DIR / toolchain_name
        if FINAL_INSTALL_DIR.exists():
            shutil.rmtree(FINAL_INSTALL_DIR)
        TEMP_INSTALL_DIR.rename(FINAL_INSTALL_DIR)

        print(f"\nBuild completed successfully in {FINAL_INSTALL_DIR}!")

    except Exception as e:
        print("\n")
        print("BUILD FAILED!")
        traceback.print_exc()
        input("\nPress Enter to exit...")
        sys.exit(1)

    input("\nPress Enter to exit...")


if __name__ == "__main__":
    main()