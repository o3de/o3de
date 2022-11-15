/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzQtComponents/Components/Widgets/CheckBox.h>

#include <FormOptionsWidget.h>

#include <QCheckBox>
#include <QFrame>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>
#include <QVBoxLayout>

namespace O3DE::ProjectManager
{

    FormOptionsWidget::FormOptionsWidget(const QString& labelText,
                                         const QStringList& options,
                                         const QString& allOptionsText,
                                         const int optionItemSpacing,
                                         QWidget* parent)
                                         : QWidget(parent)
    {
        setObjectName("formOptionsWidget");

        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setAlignment(Qt::AlignTop);
        {
            m_optionFrame = new QFrame(this);
            m_optionFrame->setObjectName("formOptionsFrame");
            QHBoxLayout* optionFrameLayout = new QHBoxLayout();
            {
                QVBoxLayout* fieldLayout = new QVBoxLayout();

                QLabel* label = new QLabel(labelText, this);
                fieldLayout->addWidget(label);


                QHBoxLayout* optionLayout = new QHBoxLayout();
                //Add the options
                for(const QString& option : options)
                {
                    QCheckBox* optionCheckBox = new QCheckBox(option);
                    optionLayout->addWidget(optionCheckBox);
                    connect(optionCheckBox, &QCheckBox::clicked, this, [=](bool checked){
                        if(checked)
                        {
                            Enable(option);
                        }
                        else
                        {
                            Disable(option);
                        }
                    });
                    m_options.insert(option, optionCheckBox);
                    optionLayout->addSpacing(optionItemSpacing);
                }

                //Add the platform option toggle
                m_allOptionsToggle = new QCheckBox(allOptionsText);
                connect(m_allOptionsToggle, &QCheckBox::clicked, this, [=](bool checked){
                    if(checked)
                    {
                        EnableAll();
                    }
                    else
                    {
                        Clear();
                    }
                });
                AzQtComponents::CheckBox::applyToggleSwitchStyle(m_allOptionsToggle);

                optionLayout->addWidget(m_allOptionsToggle);

                optionLayout->addStretch();

                fieldLayout->addLayout(optionLayout);
                optionFrameLayout->addLayout(fieldLayout);
            }

            m_optionFrame->setLayout(optionFrameLayout);
            mainLayout->addWidget(m_optionFrame);
        }
        setLayout(mainLayout);
    }

    void FormOptionsWidget::Enable(const QString& option)
    {
        if(m_options.contains(option))
        {
            auto checkbox = m_options.value(option);
            checkbox->setChecked(true);
            if(GetCheckedCount() == m_options.values().count())
            {
                m_allOptionsToggle->setChecked(true);
            }
        }
    }

    void FormOptionsWidget::Enable(const QStringList& options)
    {
        for(const QString& option : options)
        {
            Enable(option);   
        }
    }

    void FormOptionsWidget::Disable(const QString& option)
    {
        if(m_options.contains(option))
        {
            auto checkbox = m_options.value(option);
            m_allOptionsToggle->setChecked(false);
            checkbox->setChecked(false);
        }
    }

    void FormOptionsWidget::Disable(const QStringList& options)
    {
        for (const QString& option : options)
        {
            Disable(option);
        }
    }

    void FormOptionsWidget::EnableAll()
    {
        for(const auto& checkbox : m_options.values())
        {
            checkbox->setChecked(true);
        }
        m_allOptionsToggle->setChecked(true);
    }
    
    void FormOptionsWidget::Clear()
    {
        for(const auto& checkbox : m_options.values())
        {
            checkbox->setChecked(false);
        }
        m_allOptionsToggle->setChecked(false);
    }

    QStringList FormOptionsWidget::GetOptions() const
    {
        if(m_allOptionsToggle->isChecked())
        {
            return QStringList(m_options.keys());
        }

        QStringList options;
        for(const QString& option : m_options.keys())
        {
            auto checkbox = m_options.value(option);
            if(checkbox->isChecked())
            {
                options << option;
            }
        }

        return options;
    }

    int FormOptionsWidget::GetCheckedCount() const
    {
        int count = 0;
        for(const auto& checkbox : m_options.values())
        {
            count += static_cast<int>(checkbox->isChecked());
        }
        return count;
    }

} // namespace O3DE::ProjectManager
