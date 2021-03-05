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


#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>
#include <EMotionFX/Pipeline/AzSceneDef.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Data
        {
            class LodNodeSelectionList;
        }

        typedef AZStd::fixed_vector<Data::LodNodeSelectionList, g_maxLods> LODNodeLists;

        namespace Data
        {
            /*
             *  The main purpose to have a specific derived class of SceneNodeSelectionList is to have custom
             *  override attribute for the UX need of LOD
             */
            class LodNodeSelectionList
                : public SceneData::SceneNodeSelectionList
            {
            public:
                AZ_RTTI(LodNodeSelectionList, "{F19C7DD2-395C-4406-9CA9-DE572F5ADD5A}", SceneData::SceneNodeSelectionList);
                LodNodeSelectionList();
                explicit LodNodeSelectionList(AZ::u32 lodLevel);
                virtual ~LodNodeSelectionList() override = default;

                AZStd::string GetNameLabelOverride() const;
                AZ::u32 GetLODLevel() const;
                bool ContainsNode(const AZStd::string& nodeName) const;

                AZStd::unique_ptr<SceneDataTypes::ISceneNodeSelectionList> Copy() const override;

                static void Reflect(AZ::ReflectContext* context);
                
            protected:
                AZ::u32 m_lodLevel;    // Keep a reference to the owner to check the position of this list.
            };
        }
    }
}
