/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/ViewportContext.h>

#include <EMStudio/AnimViewportWidget.h>
#include <EMStudio/AnimViewportRenderer.h>

namespace EMStudio
{
    AnimViewportWidget::AnimViewportWidget(QWidget* parent)
        : AtomToolsFramework::RenderViewportWidget(parent)
    {
        setObjectName(QString::fromUtf8("AtomViewportWidget"));
        resize(869, 574);
        QSizePolicy qSize(QSizePolicy::Preferred, QSizePolicy::Preferred);
        qSize.setHorizontalStretch(0);
        qSize.setVerticalStretch(0);
        qSize.setHeightForWidth(sizePolicy().hasHeightForWidth());
        setSizePolicy(qSize);
        setAutoFillBackground(false);
        setStyleSheet(QString::fromUtf8(""));

        m_renderer = AZStd::make_unique<AnimViewportRenderer>(GetViewportContext()->GetWindowContext());
    }
}
