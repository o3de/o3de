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

#include <AzSceneDef.h>
#include <SceneAPI/SceneUI/SceneWidgets/SceneGraphWidget.h>
#include <SceneAPIExt/Data/LodNodeSelectionList.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace UI
        {
            // LODSceneGraphWidget extends the functionality of SceneGraphWidget by providing the ability to hide item that are uncheckable.
            // It also override the filtering method so that any nodes that not belongs to a certain LOD level will be filtered out.
            // Note: In a case that a node belongs to this LOD but parent node aren't, the node will create an orphan UI node in the tree structure.
            class LODSceneGraphWidget
                : public SceneUI::SceneGraphWidget
            {
            public:
                AZ_CLASS_ALLOCATOR_DECL

                LODSceneGraphWidget(const SceneContainers::Scene& scene, const SceneDataTypes::ISceneNodeSelectionList& targetList,
                    QWidget* parent = nullptr);

                // Option to hide the uncheckable item in the tree view.
                void HideUncheckableItem(bool hide);

                // Note: Some refactor is needed in the base UI function to avoid lot of code duplication. Since we are not allowed to add behaviors that tied to LOD needs
                // in the base UI, this is the best intermediate solution.
                void Build() override;

            protected:

                // Node from other LOD level will also be filtered out.
                bool IsFilteredType(const AZStd::shared_ptr<const SceneDataTypes::IGraphObject>& object,
                    SceneContainers::SceneGraph::NodeIndex index) const;

            private:

                bool m_hideUncheckableItem;
                Data::LodNodeSelectionList m_LODSelectionList;
            };
        }
    }
}
