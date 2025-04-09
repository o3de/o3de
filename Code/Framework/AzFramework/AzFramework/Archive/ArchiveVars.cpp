/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzFramework/Archive/ArchiveVars.h>

namespace AZ::IO
{
    FileSearchPriority GetDefaultFileSearchPriority()
    {
#if defined(LY_ARCHIVE_FILE_SEARCH_MODE)
        return FileSearchPriority{ LY_ARCHIVE_FILE_SEARCH_MODE };
#else
        return FileSearchPriority{ FileSearchPriority::FileFirst };
#endif
    }
}
