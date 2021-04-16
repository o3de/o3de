#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(CPACK_GENERATOR "IFW")

set(CPACK_PACKAGE_VENDOR "O3DE")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Installation Tool")

set(CPACK_PACKAGE_FILE_NAME "o3de_installer")

set(DEFAULT_LICENSE_NAME "Apache 2.0")
set(DEFAULT_LICENSE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt)

set(CPACK_RESOURCE_FILE_LICENSE ${DEFAULT_LICENSE_FILE})

set(CPACK_IFW_PACKAGE_TITLE "O3DE Installer")
set(CPACK_IFW_PACKAGE_PUBLISHER "O3DE")

set(CPACK_IFW_TARGET_DIRECTORY "@ApplicationsDir@/O3DE/${LY_VERSION_STRING}")
set(CPACK_IFW_PACKAGE_START_MENU_DIRECTORY "O3DE")

# IMPORTANT: required to be included AFTER setting all property overrides
include(CPack REQUIRED)
include(CPackIFW REQUIRED)