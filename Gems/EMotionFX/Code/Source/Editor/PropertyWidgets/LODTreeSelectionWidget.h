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
