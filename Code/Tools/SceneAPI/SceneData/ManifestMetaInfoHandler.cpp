/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneData/ManifestMetaInfoHandler.h>
#include <SceneAPI/SceneData/Groups/MeshGroup.h>
#include <SceneAPI/SceneData/Groups/SkeletonGroup.h>
#include <SceneAPI/SceneData/Groups/SkinGroup.h>
#include <SceneAPI/SceneData/Groups/AnimationGroup.h>
#include <SceneAPI/SceneData/Rules/BlendShapeRule.h>
#include <SceneAPI/SceneData/Rules/CommentRule.h>
#include <SceneAPI/SceneData/Rules/LodRule.h>
#include <SceneAPI/SceneData/Rules/MaterialRule.h>
#include <SceneAPI/SceneData/Rules/UnmodifiableRule.h>
#include <SceneAPI/SceneData/Rules/StaticMeshAdvancedRule.h>
#include <SceneAPI/SceneData/Rules/SkeletonProxyRule.h>
#include <SceneAPI/SceneData/Rules/TangentsRule.h>
#include <SceneAPI/SceneData/Rules/SkinMeshAdvancedRule.h>
#include <SceneAPI/SceneData/Rules/SkinRule.h>
#include <SceneAPI/SceneData/Rules/CoordinateSystemRule.h>
#include <SceneAPI/SceneData/Rules/TagRule.h>
#include <SceneAPI/SceneData/Rules/UVsRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            AZ_CLASS_ALLOCATOR_IMPL(ManifestMetaInfoHandler, SystemAllocator);

            ManifestMetaInfoHandler::ManifestMetaInfoHandler()
            {
                BusConnect();
            }

            ManifestMetaInfoHandler::~ManifestMetaInfoHandler()
            {
                BusDisconnect();
            }

            void ManifestMetaInfoHandler::GetAvailableModifiers(ModifiersList& modifiers, const Containers::Scene& /*scene*/, 
                const DataTypes::IManifestObject& target)
            {
                AZ_TraceContext("Object Type", target.RTTI_GetTypeName());
                modifiers.push_back(SceneData::CommentRule::TYPEINFO_Uuid());
                modifiers.push_back(SceneData::UnmodifiableRule::TYPEINFO_Uuid());

                if (target.RTTI_IsTypeOf(DataTypes::IMeshGroup::TYPEINFO_Uuid()))
                {
                    const DataTypes::IMeshGroup* group = azrtti_cast<const DataTypes::IMeshGroup*>(&target);
                    const Containers::RuleContainer& rules = group->GetRuleContainerConst();

                    AZStd::unordered_set<Uuid> existingRules;
                    const size_t ruleCount = rules.GetRuleCount();
                    for (size_t i = 0; i < ruleCount; ++i)
                    {
                        existingRules.insert(rules.GetRule(i)->RTTI_GetType());
                    }
                    if (existingRules.find(SceneData::LodRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(SceneData::LodRule::TYPEINFO_Uuid());
                    }
                    if (existingRules.find(SceneData::MaterialRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(SceneData::MaterialRule::TYPEINFO_Uuid());
                    }
                    if (existingRules.find(SceneData::StaticMeshAdvancedRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(SceneData::StaticMeshAdvancedRule::TYPEINFO_Uuid());
                    }
                    if (existingRules.find(SceneData::SkinRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(SceneData::SkinRule::TYPEINFO_Uuid());
                    }
                    if (existingRules.find(azrtti_typeid<CoordinateSystemRule>()) == existingRules.end())
                    {
                        modifiers.push_back(azrtti_typeid<CoordinateSystemRule>());
                    }
                    if (existingRules.find(SceneData::UVsRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(SceneData::UVsRule::TYPEINFO_Uuid());
                    }
                    if (existingRules.find(SceneData::TangentsRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(SceneData::TangentsRule::TYPEINFO_Uuid());
                    }
                    if (existingRules.find(SceneData::TagRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(SceneData::TagRule::TYPEINFO_Uuid());
                    }
                }
                else if (target.RTTI_IsTypeOf(DataTypes::ISkinGroup::TYPEINFO_Uuid()))
                {
                    const DataTypes::ISkinGroup* group = azrtti_cast<const DataTypes::ISkinGroup*>(&target);
                    const Containers::RuleContainer& rules = group->GetRuleContainerConst();

                    AZStd::unordered_set<AZ::Uuid> existingRules;
                    const size_t ruleCount = rules.GetRuleCount();
                    for (size_t i = 0; i < ruleCount; ++i)
                    {
                        existingRules.insert(rules.GetRule(i)->RTTI_GetType());
                    }
                    if (existingRules.find(SceneData::BlendShapeRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(SceneData::BlendShapeRule::TYPEINFO_Uuid());
                    }
                    if (existingRules.find(SceneData::LodRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(SceneData::LodRule::TYPEINFO_Uuid());
                    }
                    if (existingRules.find(SceneData::MaterialRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(SceneData::MaterialRule::TYPEINFO_Uuid());
                    }
                    if (existingRules.find(SceneData::SkinMeshAdvancedRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(SceneData::SkinMeshAdvancedRule::TYPEINFO_Uuid());
                    }

                }
                else if (target.RTTI_IsTypeOf(DataTypes::ISkeletonGroup::TYPEINFO_Uuid()))
                {
                    const DataTypes::ISkeletonGroup* group = azrtti_cast<const DataTypes::ISkeletonGroup*>(&target);
                    const Containers::RuleContainer& rules = group->GetRuleContainerConst();

                    AZStd::unordered_set<AZ::Uuid> existingRules;
                    const size_t ruleCount = rules.GetRuleCount();
                    for (size_t i = 0; i < ruleCount; ++i)
                    {
                        existingRules.insert(rules.GetRule(i)->RTTI_GetType());
                    }
                }
            }
        } // namespace SceneData
    } // namespace SceneAPI
} // namespace AZ
