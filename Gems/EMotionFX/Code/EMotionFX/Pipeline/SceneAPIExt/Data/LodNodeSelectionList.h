#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                virtual ~LodNodeSelectionList() override = default;
                bool ContainsNode(const AZStd::string& nodeName) const;

                AZStd::unique_ptr<SceneDataTypes::ISceneNodeSelectionList> Copy() const override;

                static void Reflect(AZ::ReflectContext* context);
                
            protected:
                AZ::u32 m_lodLevel;    // Keep a reference to the owner to check the position of this list.
            };
        }
    }
}
