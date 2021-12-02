/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/DocumentationLinkWidget.h>

namespace PhysX
{
    namespace Editor
    {
        DocumentationLinkWidget::DocumentationLinkWidget(const QString& format, const QString& address)
            : QLabel()
        {
            setText(format.arg(address));
            setTextInteractionFlags(Qt::TextBrowserInteraction);
            setOpenExternalLinks(true);
            setAlignment(Qt::AlignCenter);
            setContentsMargins(60, 7, 60, 7);
            setWordWrap(true);
            setStyleSheet(QString::fromUtf8("background-color: rgb(51, 51, 51);"));
        }
    }
}
