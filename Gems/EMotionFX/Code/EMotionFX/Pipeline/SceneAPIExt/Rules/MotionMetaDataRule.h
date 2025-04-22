/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPIExt/Rules/ExternalToolRule.h>

namespace EMotionFX::Pipeline::Rule
{
    struct MotionMetaData
    {
        AZ_RTTI(EMotionFX::Pipeline::Rule::MotionMetaData, "{A381A915-3CB3-4F60-82B3-70865CFA1F4F}");
        AZ_CLASS_ALLOCATOR(MotionMetaData, AZ::SystemAllocator)

        MotionMetaData();
        MotionMetaData(EMotionFX::EMotionExtractionFlags extractionFlags, EMotionFX::MotionEventTable* eventTable);
        virtual ~MotionMetaData() = default;

        EMotionFX::EMotionExtractionFlags GetMotionExtractionFlags() const { return m_motionExtractionFlags; }
        AZStd::unique_ptr<EMotionFX::MotionEventTable> GetClonedEventTable(EMotionFX::Motion* targetMotion) const;

        static void Reflect(AZ::ReflectContext* context);

    private:
        static AZStd::unique_ptr<EMotionFX::MotionEventTable> CloneMotionEventTable(EMotionFX::MotionEventTable* sourceEventTable);

        EMotionFX::EMotionExtractionFlags m_motionExtractionFlags;
        AZStd::unique_ptr<EMotionFX::MotionEventTable> m_motionEventTable;
    };

    class MotionMetaDataRule final
        : public ExternalToolRule<AZStd::shared_ptr<MotionMetaData>>
    {
    public:
        AZ_RTTI(EMotionFX::Pipeline::Rule::MotionMetaDataRule, "{E68D0C3D-CBFF-4536-95C1-676474B351A5}", AZ::SceneAPI::DataTypes::IRule);
        AZ_CLASS_ALLOCATOR(MotionMetaDataRule, AZ::SystemAllocator)

        MotionMetaDataRule();
        MotionMetaDataRule(const AZStd::shared_ptr<MotionMetaData>& data);
        ~MotionMetaDataRule() = default;

        const AZStd::shared_ptr<MotionMetaData>& GetData() const override      { return m_data; }
        void SetData(const AZStd::shared_ptr<MotionMetaData>& data) override   { m_data = data; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::shared_ptr<MotionMetaData> m_data;
    };
} // EMotionFX::Pipeline::Rule
