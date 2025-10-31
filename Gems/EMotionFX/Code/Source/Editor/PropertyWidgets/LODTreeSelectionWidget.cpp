/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            AZ_CLASS_ALLOCATOR_IMPL(LODTreeSelectionWidget, PropertyWidgetAllocator)

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
