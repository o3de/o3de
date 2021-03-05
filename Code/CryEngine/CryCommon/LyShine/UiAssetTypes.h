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
#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzFramework/Asset/SimpleAsset.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace LyShine
{
    class FontAsset
    {
    public:
        AZ_TYPE_INFO(FontAsset, "{57767D37-0EBE-43BE-8F60-AB36D2056EF8}")
        static const char* GetFileFilter()
        {
            return "*.xml;*.font;*.fontfamily";
        }
    };

    class CanvasAsset
    {
    public:
        AZ_TYPE_INFO(CanvasAsset, "{E48DDAC8-1F1E-4183-AAAB-37424BCC254B}")
        static const char* GetFileFilter()
        {
            return "*.uicanvas";
        }
    };
}
