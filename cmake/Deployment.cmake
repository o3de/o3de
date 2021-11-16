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
set(LY_ARCHIVE_FILE_SEARCH_MODE "$<$<CONFIG:release>:2>" CACHE STRING "Set the default file search mode to locate non-Pak files within the Archive System\n\
    Valid values are:\n\
    0 = Search FileSystem first, before searching within mounted Paks\n\
    1 = Search mounted Paks first, before searching FileSystem\n\
    2 = Search only mounted Paks(default in release)\n")
