#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

"""
Apply unified diffs while treating already-applied patches as success.
"""

import argparse
import logging
from pathlib import Path

import patch_ng


logger = logging.getLogger(__name__)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("patch_files", nargs="+", type=Path, help="Unified diffs to apply in order")
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")
    source_directory = Path.cwd()

    for patch_file in args.patch_files:
        try:
            patch_file = patch_file.resolve()
            if not patch_file.is_file():
                logger.error("Patch file does not exist: %s", patch_file)
                return 1

            patch_set = patch_ng.fromfile(str(patch_file))
            if not patch_set:
                logger.error("Unable to parse patch file: %s", patch_file)
                return 1

            logger.info("Applying patch %s in %s", patch_file, source_directory)
            if not patch_set.apply(root=str(source_directory)):
                logger.error("Failed to apply patch: %s", patch_file)
                return 1
        except (OSError, UnicodeError) as error:
            logger.error("Failed to apply patch %s: %s", patch_file, error)
            return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
