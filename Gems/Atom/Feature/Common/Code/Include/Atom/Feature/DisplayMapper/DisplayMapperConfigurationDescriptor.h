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

#include <Atom/Feature/ACES/Aces.h>
#include <Atom/RPI.Reflect/Pass/PassAsset.h>
#include <Atom/RPI.Reflect/Pass/PassData.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    class ReflectContext;

    namespace Render
    {
        //! A descriptor used to configure the DisplayMapper
        struct DisplayMapperConfigurationDescriptor final
        {
            AZ_TYPE_INFO(DisplayMapperConfigurationDescriptor, "{655B0C35-C96D-4EDA-810E-B50D58BC1D20}");
            static void Reflect(AZ::ReflectContext* context);

            //! Configuration name
            AZStd::string m_name;

            DisplayMapperOperationType m_operationType;

            bool m_ldrGradingLutEnabled = false;
            Data::Asset<RPI::AnyAsset> m_ldrColorGradingLut;
        };

        //! Custom pass data for DisplayMapperPass.
        struct DisplayMapperPassData
            : public RPI::PassData
        {
            using Base = RPI::PassData;
            AZ_RTTI(DisplayMapperPassData, "{2F7576F1-41C1-408A-96BF-F4B8ED280CBC}", Base);
            AZ_CLASS_ALLOCATOR(DisplayMapperPassData, SystemAllocator, 0);

            DisplayMapperPassData() = default;
            virtual ~DisplayMapperPassData() = default;

            static void Reflect(ReflectContext* context);

            DisplayMapperConfigurationDescriptor m_config;
        };

    } // namespace RPI
} // namespace AZ
