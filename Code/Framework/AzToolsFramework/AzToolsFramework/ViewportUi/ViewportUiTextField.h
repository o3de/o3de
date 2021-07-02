/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
