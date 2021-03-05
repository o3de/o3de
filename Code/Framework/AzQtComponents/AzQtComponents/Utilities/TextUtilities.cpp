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

#include <AzQtComponents/Utilities/QtWindowUtilities.h>
#include <QTextDocument>
#include <QWidget>

namespace AzQtComponents
{
    void forceToolTipLineWrap(QWidget* widget)
    {
        const QString& toolTip = widget->toolTip();

        // Qt will only line wrap tooltips if they are rich text / HTML
        // so if Qt doesn't think this text is rich text, we throw on some unnecessary
        // tags to make it think it is HTML
        if (!toolTip.isEmpty() && !Qt::mightBeRichText(toolTip))
        {
            QString newText = QStringLiteral("<b></b>%1").arg(toolTip);

            // if we're switching to HTML, we better handle newlines too
            newText = newText.replace('\n', "<br/>");

            widget->setToolTip(newText);
        }
    }

} // namespace AzQtComponents

