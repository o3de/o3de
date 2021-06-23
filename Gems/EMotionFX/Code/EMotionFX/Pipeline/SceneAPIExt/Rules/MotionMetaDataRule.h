/*
* Copyright (c) Contributors to the Open 3D Engine Project
* 
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPIExt/Rules/ExternalToolRule.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            struct MotionMetaData
            {
                AZ_RTTI(MotionMetaData, "{A381A915-3CB3-4F60-82B3-70865CFA1F4F}");
                AZ_CLASS_ALLOCATOR(MotionMetaData, AZ::SystemAllocator, 0)

                static void Reflect(AZ::ReflectContext* context);

                EMotionFX::MotionEventTable* m_motionEventTable = nullptr;
                EMotionFX::EMotionExtractionFlags m_motionExtractionFlags;
            };

            class MotionMetaDataRule
                : public ExternalToolRule<MotionMetaData>
            {
            public:
                AZ_RTTI(MotionMetaDataRule, "{E68D0C3D-CBFF-4536-95C1-676474B351A5}", AZ::SceneAPI::DataTypes::IRule);
                AZ_CLASS_ALLOCATOR(MotionMetaDataRule, AZ::SystemAllocator, 0)

                MotionMetaDataRule();
                MotionMetaDataRule(const MotionMetaData& data);

                const MotionMetaData& GetData() const override      { return m_data; }
                void SetData(const MotionMetaData& data) override   { m_data = data; }

                static void Reflect(AZ::ReflectContext* context);

            private:
                MotionMetaData m_data;
            };
        } // Rule
    } // Pipeline
} // EMotionFX
