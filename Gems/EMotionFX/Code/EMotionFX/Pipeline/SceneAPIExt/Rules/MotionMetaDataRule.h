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

#include <AzCore/Memory/SystemAllocator.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPIExt/Rules/ExternalToolRule.h>

namespace EMotionFX::Pipeline::Rule
{
    struct MotionMetaData
    {
        AZ_RTTI(EMotionFX::Pipeline::Rule::MotionMetaData, "{A381A915-3CB3-4F60-82B3-70865CFA1F4F}");
        AZ_CLASS_ALLOCATOR(MotionMetaData, AZ::SystemAllocator, 0)

        MotionMetaData() = default;
        virtual ~MotionMetaData() = default;

        static void Reflect(AZ::ReflectContext* context);

        EMotionFX::MotionEventTable* m_motionEventTable = nullptr;
        EMotionFX::EMotionExtractionFlags m_motionExtractionFlags;
    };

    class MotionMetaDataRule
        : public ExternalToolRule<MotionMetaData>
    {
    public:
        AZ_RTTI(EMotionFX::Pipeline::Rule::MotionMetaDataRule, "{E68D0C3D-CBFF-4536-95C1-676474B351A5}", AZ::SceneAPI::DataTypes::IRule);
        AZ_CLASS_ALLOCATOR(MotionMetaDataRule, AZ::SystemAllocator, 0)

        MotionMetaDataRule();
        MotionMetaDataRule(const MotionMetaData& data);
        ~MotionMetaDataRule() final = default;

        const MotionMetaData& GetData() const override      { return m_data; }
        void SetData(const MotionMetaData& data) override   { m_data = data; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        MotionMetaData m_data;
    };
} // EMotionFX::Pipeline::Rule
