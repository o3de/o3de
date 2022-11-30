/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/PaintBrush/PaintBrushSettings.h>

namespace AzToolsFramework
{
    //! Defines the specific global paintbrush settings.
    //! They can be modified directly through the Property Editor or indirectly via the GlobalPaintBrushSettingsRequestBus.
    class GlobalPaintBrushSettings : public AzFramework::PaintBrushSettings
    {
    public:
        AZ_CLASS_ALLOCATOR(GlobalPaintBrushSettings, AZ::SystemAllocator, 0);
        AZ_RTTI(GlobalPaintBrushSettings, "{524696C2-22A8-4A22-8790-D48093B71497}", PaintBrushSettings);
        static void Reflect(AZ::ReflectContext* context);

        GlobalPaintBrushSettings() = default;
        virtual ~GlobalPaintBrushSettings() = default;


    protected:

        void OnSizeRangeChanged() override;
        void OnBrushModeChanged() override;
        void OnColorModeChanged() override;

        AZ::u32 OnSettingsChanged() override;
    };

} // namespace AzToolsFramework

