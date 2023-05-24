/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/PaintBrush/PaintBrushSettings.h>
#include <AzCore/Preprocessor/Enum.h>

namespace AzToolsFramework
{
    //! The different types of functionality offered by the paint brush tool
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(
        PaintBrushMode,
        uint8_t,
        (Paintbrush, 0), //!< Uses color, opacity, and other brush settings to 'paint' values for an abstract data source
        (Eyedropper, 1), //!< Gets the current value underneath the brush from an abstract data source
        (Smooth, 2) //!< Smooths/blurs the existing values in an abstract data source
    );

    //! The different types of blend modes supported by the paint brush tool.
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(PaintBrushColorMode, uint8_t,
        (Greyscale, 0), 
        (LinearColor, 1) 
    );

    //! Defines the specific global paintbrush settings.
    //! They can be modified directly through the Property Editor or indirectly via the GlobalPaintBrushSettingsRequestBus.
    class GlobalPaintBrushSettings : public AzFramework::PaintBrushSettings
    {
    public:
        AZ_CLASS_ALLOCATOR(GlobalPaintBrushSettings, AZ::SystemAllocator);
        AZ_RTTI(GlobalPaintBrushSettings, "{524696C2-22A8-4A22-8790-D48093B71497}", PaintBrushSettings);
        static void Reflect(AZ::ReflectContext* context);

        GlobalPaintBrushSettings() = default;
        virtual ~GlobalPaintBrushSettings() = default;

        // Overall paintbrush settings

        PaintBrushMode GetBrushMode() const
        {
            return m_brushMode;
        }

        void SetBrushMode(PaintBrushMode brushMode);

        PaintBrushColorMode GetColorMode() const
        {
            return m_colorMode;
        }

        void SetColorMode(PaintBrushColorMode colorMode);


    protected:
        bool GetSizeVisibility() const override;
        bool GetColorVisibility() const override;
        bool GetIntensityVisibility() const override;
        bool GetHardnessVisibility() const override;
        bool GetFlowVisibility() const override;
        bool GetDistanceVisibility() const override;
        bool GetSmoothingRadiusVisibility() const override;
        bool GetBlendModeVisibility() const override;
        bool GetSmoothModeVisibility() const override;

        void OnBrushModeChanged();
        void OnColorModeChanged();

        void OnSizeRangeChanged() override;
        AZ::u32 OnSettingsChanged() override;

        //! Brush settings brush mode
        PaintBrushMode m_brushMode = PaintBrushMode::Paintbrush;

        //! Brush settings color mode
        PaintBrushColorMode m_colorMode = PaintBrushColorMode::Greyscale;
    };

} // namespace AzToolsFramework

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AzToolsFramework::PaintBrushMode, "{88C6AEA1-5424-4F3A-9E22-6D55C050F06C}");
    AZ_TYPE_INFO_SPECIALIZE(AzToolsFramework::PaintBrushColorMode, "{0D3B0981-BFB3-47E0-9E28-99CFB540D5AC}");
} // namespace AZ
