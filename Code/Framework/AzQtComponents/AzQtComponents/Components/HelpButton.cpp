/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/HelpButton.h>
#include <QIcon>
#include <QPixmap>
#include <QVariant>

namespace AzQtComponents
{
    HelpButton::HelpButton(QWidget* parent)
        : QPushButton(parent)
    {
        setProperty("class", QLatin1String("rounded")); // Gets styled by css
        setIcon(QPixmap(":/stylesheet/img/question.png"));
    }

} // namespace AzQtComponents

#include "Components/moc_HelpButton.cpp"
