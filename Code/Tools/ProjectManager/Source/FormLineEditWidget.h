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

#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#endif

QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QHBoxLayout)

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
        explicit FormLineEditWidget(const QString& labelText, const QString& valueText = "", QWidget* parent = nullptr);
        ~FormLineEditWidget() = default;

        //! Set the error message for to display when invalid.
        void setErrorLabelText(const QString& labelText);

        //! Returns a pointer to the underlying LineEdit.
        QLineEdit* lineEdit() const;

    protected:
        QLabel* m_errorLabel = nullptr;
        QFrame* m_frame = nullptr;
        QHBoxLayout* m_frameLayout = nullptr;
        AzQtComponents::StyledLineEdit* m_lineEdit = nullptr;

    private slots:
        void flavorChanged();
        void onFocus();
        void onFocusOut();

    private:
        void refreshStyle();
    };
} // namespace O3DE::ProjectManager
