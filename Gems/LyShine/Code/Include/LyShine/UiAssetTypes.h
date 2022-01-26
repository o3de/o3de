/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/TypeInfoSimple.h>
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
