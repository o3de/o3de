/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/ButtonStripe.h>

#include <QGridLayout>
#include <QButtonGroup>
#include <QPushButton>
#include <QVariant>

namespace AzQtComponents
{
    ButtonStripe::ButtonStripe(QWidget* parent)
        : QWidget(parent)
        , m_gridLayout(new QGridLayout(this))
        , m_buttonGroup(new QButtonGroup(this))
    {
        m_gridLayout->setSpacing(0);
    }

    void ButtonStripe::addButtons(const QStringList& buttonNames, int current)
    {
        const auto buttonsCount = buttonNames.size();
        for (int i = 0; i < buttonsCount; ++i)
        {
            auto pushButton = new QPushButton(buttonNames.at(i));
            pushButton->setCheckable(true);

            if (i == 0)
            {
                pushButton->setProperty("class", "ButtonStripeButtonFirst");
            }
            else if (i == buttonsCount - 1)
            {
                pushButton->setProperty("class", "ButtonStripeButtonLast");
            }
            else
            {
                pushButton->setProperty("class", "ButtonStripeButtonCenter");
            }

            m_buttonGroup->addButton(pushButton, i);
            m_gridLayout->addWidget(pushButton, 0, m_gridLayout->columnCount());
            m_buttons.append(pushButton);

            connect(pushButton, &QPushButton::clicked, this, [this, pushButton] {
                    emit buttonClicked(m_buttons.indexOf(pushButton));
                });
        }

        setCurrent(current);
    }

    void ButtonStripe::setCurrent(int index)
    {
        const int numButtons = m_buttons.size();
        if (index < numButtons && index >= 0)
        {
            m_buttons.at(index)->setChecked(true);
        }
    }
}

#include "Components/moc_ButtonStripe.cpp"
