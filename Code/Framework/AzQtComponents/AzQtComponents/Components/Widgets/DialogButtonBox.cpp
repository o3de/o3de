/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
