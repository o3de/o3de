/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/StyledLineEdit.h>
#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <FormComboBoxWidget.h>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QStyle>
#include <QVBoxLayout>
#include <QValidator>
#include <QComboBox>

namespace O3DE::ProjectManager
{
    FormComboBoxWidget::FormComboBoxWidget(const QString& labelText, const QStringList& items, QWidget* parent)
        : QWidget(parent)
    {
        setObjectName("formComboBoxWidget");

        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setAlignment(Qt::AlignTop);
        {
            m_frame = new QFrame(this);
            m_frame->setObjectName("formFrame");

            // use a horizontal box layout so buttons can be added to the right of the field
            m_frameLayout = new QHBoxLayout();
            {
                QVBoxLayout* fieldLayout = new QVBoxLayout();

                QLabel* label = new QLabel(labelText, this);
                fieldLayout->addWidget(label);

                m_comboBox = new QComboBox(this);
                m_comboBox->addItems(items);
                m_comboBox->setFrame(false);
                m_comboBox->installEventFilter(this);
                fieldLayout->addWidget(m_comboBox);

                m_frameLayout->addLayout(fieldLayout);
            }

            m_frame->setLayout(m_frameLayout);

            mainLayout->addWidget(m_frame);

            m_errorLabel = new QLabel(this);
            m_errorLabel->setObjectName("formErrorLabel");
            m_errorLabel->setVisible(false);
            mainLayout->addWidget(m_errorLabel);
        }

        setLayout(mainLayout);
    }

    QComboBox* FormComboBoxWidget::comboBox() const
    {
        return m_comboBox;
    }

    bool FormComboBoxWidget::eventFilter(QObject* object, QEvent* event)
    {
        if (object == m_comboBox)
        {
            if (event->type() == QEvent::FocusIn)
            {
                onFocus();
            }
            else if (event->type() == QEvent::FocusOut)
            {
                onFocusOut();
            }
        }

        return false;
    }

    void FormComboBoxWidget::setErrorLabelText(const QString& labelText)
    {
        m_errorLabel->setText(labelText);
    }

    void FormComboBoxWidget::setErrorLabelVisible(bool visible)
    {
        m_errorLabel->setVisible(visible);
        m_frame->setProperty("Valid", !visible);
        refreshStyle();
    }

    void FormComboBoxWidget::onFocus()
    {
        m_frame->setProperty("Focus", true);
        refreshStyle();
    }

    void FormComboBoxWidget::onFocusOut()
    {
        m_frame->setProperty("Focus", false);
        refreshStyle();
    }

    void FormComboBoxWidget::refreshStyle()
    {
        // we must unpolish/polish every child after changing a property
        // or else they won't use the correct stylesheet selector
        for (auto child : findChildren<QWidget*>())
        {
            child->style()->unpolish(child);
            child->style()->polish(child);
        }
    }

    void FormComboBoxWidget::mousePressEvent([[maybe_unused]] QMouseEvent* event)
    {
        m_comboBox->setFocus();
    }
} // namespace O3DE::ProjectManager

