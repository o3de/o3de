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

#if !defined(Q_MOC_RUN)
#include <QAbstractButton>
#include <QImage>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>
#endif

class QPaintEvent;
namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            /**
            ExpandCollapseToggler - Button that shows expand & collapse images and aliases
                                    "checked" state & signals with "Expanded" functions & signals 
            **/
            class SCENE_UI_API ExpandCollapseToggler : public QAbstractButton
            {
                Q_OBJECT
            public:
                explicit ExpandCollapseToggler(QWidget* parent = nullptr);
                ~ExpandCollapseToggler() override = default;

                void SetExpanded(bool isExpanded);
                bool IsExpanded() const;

            signals:
                void ExpandedChanged(bool isExpanded);

            protected:
                QSize sizeHint() const override;
                void paintEvent(QPaintEvent* evt) override; 
                const QImage* CurrentTargetImage() const;

            protected:
                QImage m_expandActionImage;
                QImage m_collapseActionImage;
            };
        } // SceneUI
    } // SceneAPI
} // AZ
