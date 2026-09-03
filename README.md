# Qt 6.12.0 — Source

This repository contains the source code for Qt 6.12.0.
It serves as archive for other projects, mainly for licensing purposes but also for a simplified build process.

The source is based on the Qt 6.12.0 source distribution obtained from the official Qt installer.

## Modifications

The following modifications were made to the Qt source used to build the distributed binaries:

- Added `#include <vector>` to /qt-6.12.0/qttools/src/linguist/shared

These changes fix build errors, they are not intended to affect behaviour.

No other modifications to the Qt source were made.

## License

See the `qt-6.12.0/LICENSES/` directory for the applicable Qt License texts and licensing information.

## Building

`build_qt.py` is provided to reproduce the Qt binaries.

The script builds the Qt 6.12.0 source contained in `qt-6.12.0/`.

The script itself is original project tooling and is not part of the Qt source distribution.
