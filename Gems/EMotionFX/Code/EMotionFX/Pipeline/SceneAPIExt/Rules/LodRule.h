#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzSceneDef.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPIExt/Data/LodNodeSelectionList.h>


namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            // The LOD rule in EMotionFX only handles skeleton. LOD information of Meshes is handled by atom.
            class LodRule
                : public SceneDataTypes::IRule
            {
            public:
                AZ_RTTI(LodRule, "{3CB103B3-CEAF-49D7-A9DC-5A31E2DF15E4}", SceneDataTypes::IRule);
                AZ_CLASS_ALLOCATOR_DECL

                ~LodRule() override = default;

                SceneData::SceneNodeSelectionList& GetSceneNodeSelectionList(size_t index);
                const SceneDataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList(size_t index) const;
                size_t GetLodRuleCount() const;

                void AddLod();

                bool ContainsNodeByPath(const AZStd::string& nodePath) const;                               // This function checks if there's any matching node path in all level
                bool ContainsNodeByRuleIndex(const AZStd::string& nodeName, AZ::u32 lodRuleIndex) const;    // This function checks if there's any matching node name in specific level.

                static void Reflect(AZ::ReflectContext* context);

            protected:
                LODNodeLists m_nodeSelectionLists;
            };
        } // SceneData
    } // SceneAPI
} // AZ
