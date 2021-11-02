// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include <AzCore/Utils/Utils.h>

#include <QLabel>
#include <QVBoxLayout>

#include <${Name}Widget.h>

namespace ${SanitizedCppName}
{
    ${SanitizedCppName}Widget::${SanitizedCppName}Widget(QWidget* parent)
        : QWidget(parent)
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        QLabel* introLabel = new QLabel(QObject::tr("Put your cool stuff here!"), this);
        mainLayout->addWidget(introLabel, 0, Qt::AlignCenter);

        QString helpText = QString(
            "For help getting started, visit the <a href=\"https://o3de.org/docs/tools-ui/ui-dev-intro/\">UI Development</a> documentation<br/>or come ask a question in the <a href=\"https://discord.gg/R77Wss3kHe\">sig-ui-ux channel</a> on Discord");

        QLabel* helpLabel = new QLabel(this);
        helpLabel->setTextFormat(Qt::RichText);
        helpLabel->setText(helpText);
        helpLabel->setOpenExternalLinks(true);

        mainLayout->addWidget(helpLabel, 0, Qt::AlignCenter);

        setLayout(mainLayout);
    }
}

#include <moc_${Name}Widget.cpp>
