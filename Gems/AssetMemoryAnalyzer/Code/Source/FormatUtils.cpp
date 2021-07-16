/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FormatUtils.h"

#include "AssetMemoryAnalyzer.h"

#include <AzFramework/StringFunc/StringFunc.h>

namespace AssetMemoryAnalyzer
{
    namespace FormatUtils
    {
        const char* FormatCodePoint(const Data::CodePoint& cp)
        {
            static char buff[1024];
            azsnprintf(buff, sizeof(buff), "%s:%d", cp.m_file, cp.m_line);

            return buff;
        }

        const char* FormatKB(size_t bytes)
        {
            static char buff[32];
            int len = azsnprintf(buff, sizeof(buff), "%0.2f", bytes / 1024.0f);
            AzFramework::StringFunc::NumberFormatting::GroupDigits(buff, sizeof(buff), len - 3);

            return buff;
        }
    }
}
