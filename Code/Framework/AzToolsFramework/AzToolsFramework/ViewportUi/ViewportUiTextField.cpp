/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzToolsFramework_precompiled.h"

#include "ViewportUiTextField.h"

#include <AzCore/Console/IConsole.h>
#include <QDoubleValidator>
#include <QIntValidator>

namespace AzToolsFramework::ViewportUi::Internal
{
    AZ_CVAR(
        int, ViewportUiTextFieldLength, 35, nullptr, AZ::ConsoleFunctorFlags::Null,
        "The pixel length of the text field part of a ViewportUiTextField");

    ViewportUiTextField::ViewportUiTextField(AZStd::shared_ptr<TextField> textField)
        : m_label(this)
        , m_lineEdit(this)
        , m_textField(textField)
    {
        setContentsMargins(0, 0, 0, 0);

        m_label.setText(textField->m_labelText.c_str());
        m_lineEdit.setText(textField->m_fieldText.c_str());

        // set the layout for the widget and settings such as alignment and margins
        auto layout = new QHBoxLayout(this);
        layout->setAlignment(Qt::AlignLeft);
        layout->addWidget(&m_label);
        layout->addWidget(&m_lineEdit);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSizeConstraint(QLayout::SetMaximumSize);

        // choose m_validator based on the validationType
        switch (textField->m_validationType)
        {
        case TextFieldValidationType::Int:
            m_validator = new QIntValidator(&m_lineEdit);
            break;
        case TextFieldValidationType::Double:
            m_validator = new QDoubleValidator(&m_lineEdit);
            break;
        case TextFieldValidationType::String:
            // nullptr is ok to pass into setValidator
            m_validator = nullptr;
            break;
        default:
            m_validator = nullptr;
        }

        m_lineEdit.setValidator(m_validator);

        connect(&m_lineEdit, &QLineEdit::textEdited, &m_lineEdit, [textField](QString text) {
            // convert the text using toLocal8Bit().data() as recommended by Qt, then emit signal
            textField->m_fieldText = text.toLocal8Bit().data();
            textField->m_textEditedEvent.Signal(textField->m_fieldText);
        });
    }

    void ViewportUiTextField::Update()
    {
        resize(minimumSizeHint());
        m_lineEdit.setFixedWidth(ViewportUiTextFieldLength);
    }
} // namespace AzToolsFramework::ViewportUi::Internal
