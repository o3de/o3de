#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Define options that control the different options for deployment for target platforms

set(LY_ASSET_DEPLOY_MODE "LOOSE" CACHE STRING "Set the Asset deployment when deploying to the target platform (LOOSE, PAK, VFS)")
set(LY_ASSET_OVERRIDE_PAK_FOLDER_ROOT "" CACHE STRING "Optional root path to where Pak file folders are stored. By default, blank will use a predefined 'paks' root.")
