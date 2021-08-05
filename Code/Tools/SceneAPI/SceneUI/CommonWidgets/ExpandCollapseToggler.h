#pragma once
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
