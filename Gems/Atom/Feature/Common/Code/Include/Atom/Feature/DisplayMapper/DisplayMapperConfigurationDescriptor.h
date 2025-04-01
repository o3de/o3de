/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include <ACES/Aces.h>

#include <Atom/RPI.Reflect/Pass/PassAsset.h>
#include <Atom/RPI.Reflect/Pass/PassData.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>

namespace AZ
{
    class ReflectContext;

    namespace Render
    {
        /**
         * The ACES display mapper parameter overrides.
         * These parameters override default ACES parameters when m_overrideDefaults is true.
         */
        struct AcesParameterOverrides final
        {
            AZ_TYPE_INFO(AcesParameterOverrides, "{3EE8C0D4-3792-46C0-B91C-B89A81C36B91}");
            static void Reflect(ReflectContext* context);

            // Load preconfigured preset for specific ODT mode defined by m_preset
            void LoadPreset();

            // When enabled allows parameter overrides for ACES configuration
            bool m_overrideDefaults = false;

            // Apply gamma adjustment to compensate for dim surround
            bool m_alterSurround = true;
            // Apply desaturation to compensate for luminance difference
            bool m_applyDesaturation = true;
            // Apply Color appearance transform (CAT) from ACES white point to assumed observer adapted white point
            bool m_applyCATD60toD65 = true;
            
            // Reference white and black luminance values
            float m_cinemaLimitsBlack = 0.02f;
            float m_cinemaLimitsWhite = 48.0f;

            // luminance linear extension below this
            float m_minPoint = 0.0028798957f;
            // luminance mid grey
            float m_midPoint = 4.8f;
            // luminance linear extension above this
            float m_maxPoint = 1005.71912f;

            // Gamma adjustment to be applied to compensate for the condition of the viewing environment.
            // Note that ACES uses a value of 0.9811 for adjusting from dark to dim surrounding.
            float m_surroundGamma = 0.9811f;
            // Optional gamma value that is applied as basic gamma curve OETF
            float m_gamma = 2.2f;

            // Allows specifying default preset for different ODT modes
            OutputDeviceTransformType m_preset = OutputDeviceTransformType_48Nits;
        };

        //! A descriptor used to configure the DisplayMapper
        struct DisplayMapperConfigurationDescriptor final
        {
            AZ_TYPE_INFO(DisplayMapperConfigurationDescriptor, "{655B0C35-C96D-4EDA-810E-B50D58BC1D20}");
            static void Reflect(AZ::ReflectContext* context);

            //! Configuration name
            AZStd::string m_name;

            DisplayMapperOperationType m_operationType = DisplayMapperOperationType::Aces;

            bool m_ldrGradingLutEnabled = false;
            Data::Asset<RPI::AnyAsset> m_ldrColorGradingLut;

            AcesParameterOverrides m_acesParameterOverrides;
        };

        //! Custom pass data for DisplayMapperPass.
        struct DisplayMapperPassData
            : public RPI::PassData
        {
            using Base = RPI::PassData;
            AZ_RTTI(DisplayMapperPassData, "{2F7576F1-41C1-408A-96BF-F4B8ED280CBC}", Base);
            AZ_CLASS_ALLOCATOR(DisplayMapperPassData, SystemAllocator);

            DisplayMapperPassData() = default;
            virtual ~DisplayMapperPassData() = default;

            static void Reflect(ReflectContext* context);

            DisplayMapperConfigurationDescriptor m_config;
            bool m_mergeLdrGradingLut = false;
            RPI::AssetReference m_outputTransformOverride;
        };
    } // namespace Render
} // namespace AZ
