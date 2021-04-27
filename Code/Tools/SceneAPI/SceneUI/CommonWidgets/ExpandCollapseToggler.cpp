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
