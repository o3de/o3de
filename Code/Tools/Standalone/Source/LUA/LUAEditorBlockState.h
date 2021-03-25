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

#include <AzCore/base.h>

#pragma once

namespace LUAEditor
{
    union QTBlockState
    {
        struct BlockState
        {
            AZ::u32 m_uninitialized : 1; //using semantic negative here. because qt by default will set our int to -1 and we need to detect that
            AZ::u32 m_folded : 1; //have be careful with folded, it is handled differently, syntax highlighter must preserve existing value of it.
            AZ::u32 m_foldLevel : 14;
            AZ::u32 m_syntaxHighlighterState : 3;
            AZ::u32 m_syntaxHighlighterStateExtra : 13;
        };
        BlockState m_blockState;
        int m_qtBlockState;
    };
    static_assert(sizeof(QTBlockState) == sizeof(int), "QT stores block state in an int");
}