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
#include "AssetMemoryAnalyzer_precompiled.h"

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
