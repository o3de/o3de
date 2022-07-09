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

namespace EMotionFX::Pipeline::Rule
{
    struct RootMotionExtractionData
    {
        AZ_RTTI(EMotionFX::Pipeline::Rule::RootMotionExtractionData, "{7AA82E47-88CC-4430-9AEE-83BFB671D286}");
        AZ_CLASS_ALLOCATOR(RootMotionExtractionData, AZ::SystemAllocator, 0)

        virtual ~RootMotionExtractionData() = default;
        static void Reflect(AZ::ReflectContext* context);

        bool m_transitionZeroXAxis = false;
        bool m_transitionZeroYAxis = false;
        bool m_extractRotation = false;
        AZStd::string m_sampleJoint = "Hip";
    };

    class RootMotionExtractionRule final
        : public ExternalToolRule<RootMotionExtractionData>
    {
    public:
        AZ_RTTI(EMotionFX::Pipeline::Rule::RootMotionExtractionRule, "{1A7E6215-49E3-4D80-8B5C-1DA8E09DA5FB}", AZ::SceneAPI::DataTypes::IRule);
        AZ_CLASS_ALLOCATOR(RootMotionExtractionRule, AZ::SystemAllocator, 0)

        RootMotionExtractionRule() = default;
        RootMotionExtractionRule(const RootMotionExtractionData& data);
        ~RootMotionExtractionRule() = default;

        const RootMotionExtractionData& GetData() const override { return m_data; }
        void SetData(const RootMotionExtractionData& data) override { m_data = data; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        RootMotionExtractionData m_data;
    };
} // EMotionFX::Pipeline::Rule
