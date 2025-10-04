/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Window/Controls/InfoRadioButton.h>

#include <QHBoxLayout>
#include <QRadioButton>
#include <QDebug>

namespace OpenParticleSystemEditor
{
    InfoRadioButton::InfoRadioButton(const QString& text, int elementCount, QWidget* parent)
        : QWidget(parent)
    {
        SetupUI(text, elementCount);
    }

    InfoRadioButton::~InfoRadioButton()
    {
    }

    void InfoRadioButton::SetupUI(const QString& text, int elementCount)
    {
        QHBoxLayout* layout = new QHBoxLayout();
        layout->setAlignment(Qt::AlignLeft);

        m_label = new QLabel();
        m_label->setContentsMargins(0, 0, 0, 0);
        m_label->setMinimumWidth(32);
        m_label->setText(text);
        layout->addWidget(m_label);

        m_buttonGroup = new QButtonGroup(this);
        m_buttonGroup->setExclusive(true);
        auto addRadioFun = [this, layout](const AZStd::string& name)
        {
            QRadioButton* button = new QRadioButton(this);
            button->setText(name.data());
            layout->addWidget(button);
            m_buttonGroup->addButton(button);
        };
        if (elementCount == 1)
        {
            addRadioFun("X");
        }
        else
        {
            addRadioFun("X");
            addRadioFun("Y");
            addRadioFun("Z");
        }

        layout->setContentsMargins(0, 0, 0, 0);
        setLayout(layout);

        connect(m_buttonGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(onButtonClicked(QAbstractButton*)));
    }

    void InfoRadioButton::onButtonClicked([[maybe_unused]] QAbstractButton* button)
    {
        QList<QAbstractButton*> buttonList = m_buttonGroup->buttons();
        for (int i = 0; i < buttonList.length(); i++)
        {
            QAbstractButton* btn = buttonList.at(i);
            if (btn->isChecked())
            {
                emit radioButtonChecked(i);
                break;
            }
        }
    }

    void InfoRadioButton::SetCurrentChecked(int index)
    {
        QList<QAbstractButton*> buttonList = m_buttonGroup->buttons();
        for (int i = 0; i < buttonList.length(); i ++)
        {
            QAbstractButton* button = buttonList.at(i);
            button->setChecked(i == index ? true : false);
        }
    }
} // namespace OpenParticleSystemEditor
