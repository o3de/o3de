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
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPIExt/Rules/ExternalToolRule.h>
#include <EMotionFX/Source/MotionData/RootMotionExtractionData.h>

namespace EMotionFX::Pipeline::Rule
{
    class RootMotionExtractionRule final
        : public ExternalToolRule<AZStd::shared_ptr<RootMotionExtractionData>>
    {
    public:
        AZ_RTTI(EMotionFX::Pipeline::Rule::RootMotionExtractionRule, "{1A7E6215-49E3-4D80-8B5C-1DA8E09DA5FB}", AZ::SceneAPI::DataTypes::IRule);
        AZ_CLASS_ALLOCATOR(RootMotionExtractionRule, AZ::SystemAllocator)

        RootMotionExtractionRule();
        RootMotionExtractionRule(AZStd::shared_ptr<RootMotionExtractionData> data);
        ~RootMotionExtractionRule() = default;

        const AZStd::shared_ptr<RootMotionExtractionData>& GetData() const override { return m_data; }
        void SetData(const AZStd::shared_ptr<RootMotionExtractionData>& data) override { m_data = data; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::shared_ptr<RootMotionExtractionData> m_data;
    };
} // EMotionFX::Pipeline::Rule
