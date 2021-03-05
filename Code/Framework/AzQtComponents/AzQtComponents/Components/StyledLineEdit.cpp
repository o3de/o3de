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

#include <AzQtComponents/Components/StyledLineEdit.h>
#include <QAction>
#include <QValidator>

namespace AzQtComponents
{
    StyledLineEdit::StyledLineEdit(QWidget* parent)
        : QLineEdit(parent)
        , m_flavor(Plain)
    {
        setFlavor(Plain);
        connect(this, &StyledLineEdit::textChanged, this, &StyledLineEdit::validateEntry);
    }

    StyledLineEdit::~StyledLineEdit()
    {
    }

    StyledLineEdit::Flavor StyledLineEdit::flavor() const
    {
        return m_flavor;
    }

    void StyledLineEdit::setFlavor(StyledLineEdit::Flavor flavor)
    {
        if (flavor != m_flavor)
        {
            m_flavor = flavor;
            emit flavorChanged();
        }
    }

    void StyledLineEdit::focusInEvent(QFocusEvent* event)
    {
        emit(onFocus()); // Required for focus dependent custom widgets, e.g. ConfigStringLineEditCtrl.
        QLineEdit::focusInEvent(event);
    }

    void StyledLineEdit::focusOutEvent(QFocusEvent* event)
    {
        emit(onFocusOut());
        QLineEdit::focusOutEvent(event);
    }

    void StyledLineEdit::validateEntry()
    {
        QString textToValidate = text();
        int length = textToValidate.length();

        if (!validator())
        {
            return;
        }

        if (validator()->validate(textToValidate, length) == QValidator::Acceptable && length > 0)
        {
            setFlavor(StyledLineEdit::Valid);
        }

        else if (validator()->validate(textToValidate, length) == QValidator::Acceptable && length <= 0)
        {
            setFlavor(StyledLineEdit::Plain);
        }
        else
        {
            setFlavor(StyledLineEdit::Invalid);
        }
    }

} // namespace AzQtComponents

#include "Components/moc_StyledLineEdit.cpp"
