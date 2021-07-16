/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneUI/CommonWidgets/ExpandCollapseToggler.h>
#include <QPainter>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            ExpandCollapseToggler::ExpandCollapseToggler(QWidget* parent)
                : QAbstractButton(parent)
                , m_expandActionImage(":/SceneUI/Common/ExpandIcon.png")
                , m_collapseActionImage(":/SceneUI/Common/CollapseIcon.png")
            {
                setCheckable(true);
                connect(this, &ExpandCollapseToggler::toggled, this, &ExpandCollapseToggler::ExpandedChanged);
            }

            void ExpandCollapseToggler::SetExpanded(bool isExpanded)
            {
                setChecked(isExpanded);
            }

            bool ExpandCollapseToggler::IsExpanded() const
            {
                return isChecked();
            }

            const QImage* ExpandCollapseToggler::CurrentTargetImage() const
            {
                return IsExpanded() ? &m_collapseActionImage : &m_expandActionImage;
            }

            QSize ExpandCollapseToggler::sizeHint() const
            {
                return CurrentTargetImage()->size();
            }

            void ExpandCollapseToggler::paintEvent([[maybe_unused]] QPaintEvent* evt)
            {
                QPainter painter(this);
                const QImage* target = CurrentTargetImage();
                painter.drawImage(QPoint(0,0), *target);
            }
        } // SceneUI
    } // SceneAPI
} // AZ

#include <CommonWidgets/moc_ExpandCollapseToggler.cpp>
