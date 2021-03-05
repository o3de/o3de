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
