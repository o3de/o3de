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

#include <PhysX_precompiled.h>

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
            setContentsMargins(60, 15, 60, 15);
            setWordWrap(true);
        }
    }
}