/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/vector.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IBlendShapeRule.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            class MorphTargetRule
                : public AZ::SceneAPI::DataTypes::IBlendShapeRule
            {
            public:
                AZ_RTTI(MorphTargetRule, "{B27836D7-B76C-4797-A74A-F0C29B9E056C}", AZ::SceneAPI::DataTypes::IBlendShapeRule);
                AZ_CLASS_ALLOCATOR_DECL

                MorphTargetRule();
                ~MorphTargetRule() override = default;

                AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() override;
                const AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() const override;

                static void Reflect(AZ::ReflectContext* context);

                void OnUserAdded() override;
                void OnUserRemoved() const override;

                // Utility function.
                static size_t SelectMorphTargets(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& selection);

            protected:
                bool                                                m_readOnly;
                AZ::SceneAPI::SceneData::SceneNodeSelectionList     m_morphTargets;
            };

            // A read only version of Morph target rule. Used in MotionGroup that only displays the count.
            class MorphTargetRuleReadOnly
                : public AZ::SceneAPI::DataTypes::IRule
            {
            public:
                AZ_RTTI(MorphTargetRuleReadOnly, "{A02248AC-F37A-47A1-9814-C5136E9133D8}", IRule);
                AZ_CLASS_ALLOCATOR_DECL

                MorphTargetRuleReadOnly();
                MorphTargetRuleReadOnly(size_t morphAnimationCount);

                void SetMorphAnimationCount(size_t morphAnimationCount);
                size_t GetMorphAnimationCount() const;

                static void Reflect(AZ::ReflectContext* context);

                static size_t DetectMorphTargetAnimations(const AZ::SceneAPI::Containers::Scene& scene);

            protected:
                AZ::u64 m_morphAnimationCount;
                AZStd::string m_descriptionText;
            };

            static const char* MORPH_TARGET_RULE_ADD_METRIC_EVENT_NAME = "MorphRuleAdded";
            static const char* MORPH_TARGET_RULE_REMOVE_METRIC_EVENT_NAME = "MorphRuleRemoved";

        } // Rule
    } // Pipeline
} // EMotionFX
