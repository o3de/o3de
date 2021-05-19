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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Color.h>

#include <AzFramework/Viewport/ViewportId.h>

namespace AzToolsFramework
{
    class EditorViewportIconDisplayInterface
    {
    public:
        AZ_RTTI(EditorViewportIconDisplayInterface, "{D5190B58-2561-4F3F-B793-F1E7D454CDF2}");

        using IconId = AZ::s32;
        static constexpr IconId InvalidIconId = -1;

        struct DrawParameters
        {
            AzFramework::ViewportId m_viewport = AzFramework::InvalidViewportId;
            IconId m_icon = InvalidIconId;
            AZ::Color m_color = AZ::Colors::White;
            AZ::Vector3 m_position;
            AZ::Vector2 m_size;
        };

        enum class IconLoadStatus : AZ::u8
        {
            Unloaded,
            Loading,
            Loaded,
            Error
        };

        virtual void DrawIcon(const DrawParameters& drawParameters) = 0;
        virtual IconId GetOrLoadIconForPath(AZStd::string_view path) = 0;
        virtual IconLoadStatus GetIconLoadStatus(IconId icon) = 0;
    };

    using EditorViewportIconDisplay = AZ::Interface<EditorViewportIconDisplayInterface>;
} //namespace AzToolsFramework
