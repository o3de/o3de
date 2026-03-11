#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# https://cmake.org/cmake/help/latest/policy/CMP0135.html
if(POLICY CMP0135)
    # Ensure extracted archive files use the extraction time as their timestamp.
    # This avoids embedding historical or non-deterministic timestamps from the archive itself,
    # which improves build reproducibility and prevents unnecessary rebuilds
    # when timestamps differ but file contents do not.
    cmake_policy(SET CMP0135 NEW)
endif()

# https://cmake.org/cmake/help/latest/policy/CMP0156.html
if(POLICY CMP0156)
    # Automatically remove redundant libraries from link lines when the linker supports it.
    cmake_policy(SET CMP0156 NEW)
endif()

# https://cmake.org/cmake/help/latest/policy/CMP0168.html
if(POLICY CMP0168)
    # Perform FetchContent downloads directly during the configure step
    # rather than using separate sub-builds.
    cmake_policy(SET CMP0168 NEW)
endif()

# https://cmake.org/cmake/help/latest/policy/CMP0179.html
if(POLICY CMP0179)
    # When de-duplicating static libraries on the link line,
    # keep the first occurrence and discard later duplicates.
    cmake_policy(SET CMP0179 NEW)
endif()
