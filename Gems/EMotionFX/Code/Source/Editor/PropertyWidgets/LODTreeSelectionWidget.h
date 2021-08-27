/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <SceneAPI/SceneUI/RowWidgets/NodeTreeSelectionWidget.h>
#include <EMotionFX/Pipeline/AzSceneDef.h>
#endif

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace UI
        {
            class LODTreeSelectionWidget : public SceneUI::NodeTreeSelectionWidget
            {
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR_DECL
                LODTreeSelectionWidget(QWidget* parent);

                void HideUncheckable(bool hide);

            protected:

                virtual void ResetNewTreeWidget(const SceneContainers::Scene& scene);

                bool m_hideUncheckableItem;
            };
        }
    }
}
