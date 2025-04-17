#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(LY_ARCHIVE_FILE_SEARCH_MODE "" CACHE STRING "Set the default file search mode to locate non-Pak files within the Archive System\n\
    Valid values are:\n\
    0 = Search FileSystem first, before searching within mounted Paks (default in debug/profile/release)\n\
    1 = Search mounted Paks first, before searching FileSystem\n\
    2 = Search only mounted Paks\n")
