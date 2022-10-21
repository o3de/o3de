/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#endif

QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QMovie)
QT_FORWARD_DECLARE_CLASS(QHBoxLayout)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QMouseEvent)

namespace AzQtComponents
{
    class StyledLineEdit;
}

namespace O3DE::ProjectManager
{
    class FormLineEditWidget
        : public QWidget 
    {
        Q_OBJECT

    public:

        enum class ValidationState
        {
            NotValidating,
            Validating,
            ValidationFailed,
            ValidationSuccess
        };

        FormLineEditWidget(
            const QString& labelText,
            const QString& valueText,
            const QString& placeholderText,
            const QString& errorText,
            QWidget* parent = nullptr);
        explicit FormLineEditWidget(const QString& labelText, const QString& valueText = "", QWidget* parent = nullptr);
        ~FormLineEditWidget() = default;

        //! Set the error message for to display when invalid.
        void setErrorLabelText(const QString& labelText);
        void setErrorLabelVisible(bool visible);

        //! Returns a pointer to the underlying LineEdit.
        QLineEdit* lineEdit() const;

        virtual void setText(const QString& text);

        void SetValidationState(ValidationState validationState);

    protected:
        QLabel* m_errorLabel = nullptr;
        QFrame* m_frame = nullptr;
        QHBoxLayout* m_frameLayout = nullptr;
        AzQtComponents::StyledLineEdit* m_lineEdit = nullptr;
        QVBoxLayout* m_mainLayout = nullptr;

        //Validation icons
        QMovie* m_processingSpinnerMovie = nullptr;
        QLabel* m_processingSpinner = nullptr;
        QLabel* m_validationErrorIcon = nullptr;
        QLabel* m_validationSuccessIcon = nullptr;
        ValidationState m_validationState = ValidationState::NotValidating;
        void refreshStyle();

    private slots:
        void flavorChanged();
        void onFocus();
        void onFocusOut();

    private:
        void mousePressEvent(QMouseEvent* event) override;
        
    };
} // namespace O3DE::ProjectManager
