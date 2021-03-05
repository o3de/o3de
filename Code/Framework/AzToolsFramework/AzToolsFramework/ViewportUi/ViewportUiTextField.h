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
#include <AzToolsFramework/ViewportUi/TextField.h>
#include <QLabel>
#include <QLineEdit>
#include <QWidget>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <QHBoxLayout>
#include <QVBoxLayout>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework::ViewportUi::Internal
{
    //! Helper class for a widget that holds and manages multiple LabelTextFields.
    class ViewportUiTextField
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit ViewportUiTextField(AZStd::shared_ptr<TextField> textField);
        ~ViewportUiTextField() = default;

        void Update();

    private:
        QLabel m_label; //<! The text label.
        QLineEdit m_lineEdit; //<! The editable text field.
        QValidator* m_validator; //<! The validator for the line edit text.
        AZStd::shared_ptr<TextField> m_textField; //<! Reference to the text field data struct.
    };
} // namespace AzToolsFramework::ViewportUi::Internal
