/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <FormLineEditWidget.h>
#include <AzQtComponents/Components/StyledLineEdit.h>
#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QMovie>
#include <QFrame>
#include <QValidator>
#include <QStyle>

namespace O3DE::ProjectManager
{
    FormLineEditWidget::FormLineEditWidget(
        const QString& labelText,
        const QString& valueText,
        const QString& placeholderText,
        const QString& errorText,
        QWidget* parent)
        : QWidget(parent)
        
    {
        setObjectName("formLineEditWidget");

        m_mainLayout = new QVBoxLayout();
        m_mainLayout->setAlignment(Qt::AlignTop);
        {
            m_frame = new QFrame(this);
            m_frame->setObjectName("formFrame");

            // use a horizontal box layout so buttons can be added to the right of the field
            m_frameLayout = new QHBoxLayout();
            {
                QVBoxLayout* fieldLayout = new QVBoxLayout();

                QLabel* label = new QLabel(labelText, this);
                fieldLayout->addWidget(label);

                m_lineEdit = new AzQtComponents::StyledLineEdit(this);
                m_lineEdit->setFlavor(AzQtComponents::StyledLineEdit::Question);
                AzQtComponents::LineEdit::setErrorIconEnabled(m_lineEdit, false);
                m_lineEdit->setText(valueText);
                m_lineEdit->setPlaceholderText(placeholderText);

                connect(m_lineEdit, &AzQtComponents::StyledLineEdit::flavorChanged, this, &FormLineEditWidget::flavorChanged);
                connect(m_lineEdit, &AzQtComponents::StyledLineEdit::onFocus, this, &FormLineEditWidget::onFocus);
                connect(m_lineEdit, &AzQtComponents::StyledLineEdit::onFocusOut, this, &FormLineEditWidget::onFocusOut);

                m_lineEdit->setFrame(false);
                fieldLayout->addWidget(m_lineEdit);

                m_frameLayout->addLayout(fieldLayout);

                QWidget* emptyWidget = new QWidget(this);
                m_frameLayout->addWidget(emptyWidget);

                m_processingSpinnerMovie = new QMovie(":/in_progress.gif");
                m_processingSpinner = new QLabel(this);
                m_processingSpinner->setScaledContents(true);
                m_processingSpinner->setMaximumSize(32, 32);
                m_processingSpinner->setMovie(m_processingSpinnerMovie);
                m_frameLayout->addWidget(m_processingSpinner);

                m_validationErrorIcon = new QLabel(this);
                m_validationErrorIcon->setPixmap(QIcon(":/error.svg").pixmap(32, 32));
                m_frameLayout->addWidget(m_validationErrorIcon);

                m_validationSuccessIcon = new QLabel(this);
                m_validationSuccessIcon->setPixmap(QIcon(":/checkmark.svg").pixmap(32, 32));
                m_frameLayout->addWidget(m_validationSuccessIcon);

                SetValidationState(ValidationState::NotValidating);
            }

            m_frame->setLayout(m_frameLayout);

            m_mainLayout->addWidget(m_frame);

            m_errorLabel = new QLabel(this);
            m_errorLabel->setObjectName("formErrorLabel");
            m_errorLabel->setText(errorText);
            m_errorLabel->setVisible(false);
            m_mainLayout->addWidget(m_errorLabel);
        }

        setLayout(m_mainLayout);
    }

    FormLineEditWidget::FormLineEditWidget(const QString& labelText, const QString& valueText, QWidget* parent)
        : FormLineEditWidget(labelText, valueText, "", "", parent)
    {
    }

    void FormLineEditWidget::setErrorLabelText(const QString& labelText)
    {
        m_errorLabel->setText(labelText);
    }

    void FormLineEditWidget::setErrorLabelVisible(bool visible)
    {
        m_errorLabel->setVisible(visible);
        m_frame->setProperty("Valid", !visible);

        refreshStyle();
    }

    QLineEdit* FormLineEditWidget::lineEdit() const
    {
        return m_lineEdit;
    }

    void FormLineEditWidget::flavorChanged()
    {
        if (m_lineEdit->flavor() == AzQtComponents::StyledLineEdit::Flavor::Invalid)
        {
            m_frame->setProperty("Valid", false);
            m_errorLabel->setVisible(true);
        }
        else
        {
            m_frame->setProperty("Valid", true);
            m_errorLabel->setVisible(false);
        }
        refreshStyle();
    }

    void FormLineEditWidget::onFocus()
    {
        m_frame->setProperty("Focus", true);
        refreshStyle();
    }

    void FormLineEditWidget::onFocusOut()
    {
        m_frame->setProperty("Focus", false);
        refreshStyle();
    }

    void FormLineEditWidget::refreshStyle()
    {
        // we must unpolish/polish every child after changing a property
        // or else they won't use the correct stylesheet selector
        for (auto child : findChildren<QWidget*>())
        {
            child->style()->unpolish(child);
            child->style()->polish(child);
        }
    }

    void FormLineEditWidget::setText(const QString& text)
    {
        m_lineEdit->setText(text);
    }

    void FormLineEditWidget::SetValidationState(ValidationState validationState)
    {
        switch (validationState)
        {
        case ValidationState::Validating:
            m_processingSpinnerMovie->start();
            m_processingSpinner->setVisible(true);
            m_validationErrorIcon->setVisible(false);
            m_validationSuccessIcon->setVisible(false);
            break;
        case ValidationState::ValidationSuccess:
            m_processingSpinnerMovie->stop();
            m_processingSpinner->setVisible(false);
            m_validationErrorIcon->setVisible(false);
            m_validationSuccessIcon->setVisible(true);
            break;
        case ValidationState::ValidationFailed:
            m_processingSpinnerMovie->stop();
            m_processingSpinner->setVisible(false);
            m_validationErrorIcon->setVisible(true);
            m_validationSuccessIcon->setVisible(false);
            break;
        case ValidationState::NotValidating:
        default:
            m_processingSpinnerMovie->stop();
            m_processingSpinner->setVisible(false);
            m_validationErrorIcon->setVisible(false);
            m_validationSuccessIcon->setVisible(false);
            break;
        }
    }

    void FormLineEditWidget::mousePressEvent([[maybe_unused]] QMouseEvent* event)
    {
        m_lineEdit->setFocus();
    }
} // namespace O3DE::ProjectManager
