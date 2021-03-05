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

#include <AzQtComponents/Components/Widgets/DialogButtonBox.h>
#include <AzQtComponents/Components/Style.h>

#include <QDialogButtonBox>
#include <QPushButton>

namespace AzQtComponents
{

bool DialogButtonBox::polish(Style* style, QWidget* widget)
{
    auto dialogButtonBox = qobject_cast<QDialogButtonBox*>(widget);
    if (dialogButtonBox)
    {
        // We need to repolish any push buttons on a QDialogButtonBox when it
        // is polished, because it might have changed the default button before
        // it was shown
        for (auto button : dialogButtonBox->buttons())
        {
            auto dialogPushButton = qobject_cast<QPushButton*>(button);
            if (dialogPushButton)
            {
                style->unpolish(dialogPushButton);
                style->polish(dialogPushButton);
            }
        }

        return true;
    }

    return false;
}

} // namespace AzQtComponents
