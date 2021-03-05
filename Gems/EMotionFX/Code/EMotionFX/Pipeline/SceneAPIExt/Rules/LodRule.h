#pragma once

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
