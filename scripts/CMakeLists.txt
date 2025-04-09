#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    add_subdirectory(commit_validation)
    add_subdirectory(ctest)
    add_subdirectory(detect_file_changes)
    add_subdirectory(license_scanner)
    add_subdirectory(o3de)
endif()
