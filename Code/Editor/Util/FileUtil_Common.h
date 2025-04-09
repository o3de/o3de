/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "../Include/IFileUtil.h"

namespace Common
{
    enum EditFileType
    {
        FILE_TYPE_SCRIPT = IFileUtil::FILE_TYPE_SCRIPT,
        FILE_TYPE_SHADER = IFileUtil::FILE_TYPE_SHADER,
        FILE_TYPE_BSPACE = IFileUtil::FILE_TYPE_BSPACE,
        FILE_TYPE_TEXTURE,
        FILE_TYPE_ANIMATION
    };

    bool Exists(const QString& strPath, bool boDirectory, IFileUtil::FileDesc* pDesc = nullptr);
    bool PathExists(const QString& strPath);
}
