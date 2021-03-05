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

#include <Editor/PropertyWidgets/LODSceneGraphWidget.h>
#include <Editor/PropertyWidgets/LODTreeSelectionWidget.h>
#include <Editor/PropertyWidgets/PropertyWidgetAllocator.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace UI
        {
            AZ_CLASS_ALLOCATOR_IMPL(LODTreeSelectionWidget, PropertyWidgetAllocator, 0)

            LODTreeSelectionWidget::LODTreeSelectionWidget(QWidget* parent)
                : NodeTreeSelectionWidget(parent)
                , m_hideUncheckableItem(false)
            {
            }

            void LODTreeSelectionWidget::HideUncheckable(bool hide)
            {
                m_hideUncheckableItem = hide;
            }

            void LODTreeSelectionWidget::ResetNewTreeWidget(const SceneContainers::Scene& scene)
            {
                LODSceneGraphWidget* widget = aznew LODSceneGraphWidget(scene, *m_list);
                widget->HideUncheckableItem(m_hideUncheckableItem);
                m_treeWidget.reset(widget);
            }
        }
    }
}

#include <Source/Editor/PropertyWidgets/moc_LODTreeSelectionWidget.cpp>
