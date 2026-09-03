#!/usr/bin/env python3
import os
import shutil
import subprocess
import sys
from pathlib import Path
import traceback


# ============================================================
# Global configuration
# ============================================================

COMPILER_BIN = Path(r"...\bin")
QT_SOURCE = Path(__file__).resolve().parent / "qt"
BUILD_DIR = Path(__file__).resolve().parent / "build"
INSTALL_PREFIX = Path(__file__).resolve().parent / "install"


C_COMPILER = COMPILER_BIN / "clang.exe"
CXX_COMPILER = COMPILER_BIN / "clang++.exe"

CMAKE_BIN = None    # None: get from PATH
NINJA_BIN = None    # None: get from PATH

# Qt configure options
QT_SUBMODULES = "qtbase,qttools"
QT_USE_ORIGINAL_COMPILER = True # if true, the generated qt.toolchain.cmake file will set C/CXX to the compiler used to build Qt itself

SYSTEM_PATHS = [
    os.path.join(os.environ["SystemRoot"], "System32"),
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

def find_tool(tool):
    """Find a tool in the current global PATH."""
    result = shutil.which(tool)
    if result is None:
        raise RuntimeError(f"{tool} not found in PATH")
    return result

def create_build_environment():
    env = os.environ.copy()

    path_entries = []

    path_entries.append(COMPILER_BIN)

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
        # Create build directory
        # ----------------------------------------------------
        Path(BUILD_DIR).mkdir( parents=True, exist_ok=True)

        # ----------------------------------------------------
        # Configure Qt
        # ----------------------------------------------------
        configure_cmd = [
            QT_SOURCE / "configure.bat",

            "-prefix", INSTALL_PREFIX,

            "-release",
            "-shared", "-force-debug-info", "-separate-debug-info", # shared build
            "-opensource",
            "-confirm-license",

            "-cmake-generator", "Ninja",

            "-submodules", QT_SUBMODULES,

            "--",

            f"-DCMAKE_C_COMPILER={C_COMPILER}",
            f"-DCMAKE_CXX_COMPILER={CXX_COMPILER}",
            "-DQT_USE_ORIGINAL_COMPILER=ON" if QT_USE_ORIGINAL_COMPILER else "",
            "-DQT_INSTALL_CONFIG_INFO_FILES=ON",

            # "-DCMAKE_C_COMPILER_TARGET=x86_64-w64-windows-gnu",
            # "-DCMAKE_CXX_COMPILER_TARGET=x86_64-w64-windows-gnu",
        ]

        run_command(configure_cmd, cwd=BUILD_DIR, env=env)

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

        print("\nBuild completed successfully!")

    except Exception as e:
        print("\n")
        print("BUILD FAILED!")
        traceback.print_exc()
        input("\nPress Enter to exit...")
        sys.exit(1)

    input("\nPress Enter to exit...")


if __name__ == "__main__":
    main()