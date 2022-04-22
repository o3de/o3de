#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#


ly_install_files(FILES license_scanner.py PROGRAMS
    DESTINATION scripts/license_scanner
)
ly_install_files(FILES scanner_config.json
    DESTINATION scripts/license_scanner
)
