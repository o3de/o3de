/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_UPTODATEFILEHELPERS_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_UPTODATEFILEHELPERS_H
#pragma once

#include <assert.h>
#include "FileUtil.h"
#include "IRCLog.h"  // RCLog

namespace UpToDateFileHelpers
{
    inline bool FileExistsAndUpToDate(
        const char* const pDstFileName,
        const char* const pSrcFileName)
    {
        const FILETIME dstFileTime = FileUtil::GetLastWriteFileTime(pDstFileName);
        if (!FileUtil::FileTimeIsValid(dstFileTime))
        {
            return false;
        }

        const FILETIME srcFileTime = FileUtil::GetLastWriteFileTime(pSrcFileName);
        if (!FileUtil::FileTimeIsValid(srcFileTime))
        {
            RCLogWarning("%s: Source file \"%s\" doesn't exist", __FUNCTION__, pSrcFileName);
            return false;
        }

        return FileUtil::FileTimesAreEqual(srcFileTime, dstFileTime);
    }


    inline bool SetMatchingFileTime(
        const char* const pDstFileName,
        const char* const pSrcFileName)
    {
        const FILETIME srcFileTime = FileUtil::GetLastWriteFileTime(pSrcFileName);
        if (!FileUtil::FileTimeIsValid(srcFileTime))
        {
            RCLogError("%s: Source file \"%s\" doesn't exist", __FUNCTION__, pSrcFileName);
            return false;
        }

        const FILETIME dstFileTime = FileUtil::GetLastWriteFileTime(pDstFileName);
        if (!FileUtil::FileTimeIsValid(dstFileTime))
        {
            RCLogError("%s: Destination file \"%s\" doesn't exist", __FUNCTION__, pDstFileName);
            return false;
        }

        if (!FileUtil::SetFileTimes(pSrcFileName, pDstFileName))
        {
            RCLogError("%s: Copying the date and time from \"%s\" to \"%s\" failed", __FUNCTION__, pSrcFileName, pDstFileName);
            return false;
        }

        assert(FileExistsAndUpToDate(pDstFileName, pSrcFileName));
        return true;
    }
} // namespace UpToDateFileHelpers

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_UPTODATEFILEHELPERS_H
