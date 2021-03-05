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
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/functional.h>
#include <QLineEdit>
#include <QRegExpValidator>
#endif

namespace EMStudio
{
    class LineEditValidatable
        : public QLineEdit
    {
        Q_OBJECT //AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR_DECL

        explicit LineEditValidatable(QWidget* parent, const QRegExp& regExp = s_defaultRegExp);
        ~LineEditValidatable() override = default;

        void SetPreviousText(const QString& previousText);
        QString GetPreviousText() const;

        void SetValidatorFunc(const AZStd::function<bool()>& func) { m_validatorFunc = func; }
        bool IsValid() const;

        static const QRegExp s_defaultRegExp;

    signals:
        void TextEditingFinished();
        void TextChanged();

    public slots:
        void OnTextChanged();
        void OnEditingFinished();

    private:
        void focusInEvent(QFocusEvent* event) override;

        QString m_previousText;

        QRegExp m_validationExp;
        QRegExpValidator m_lineValidator;
        AZStd::function<bool()> m_validatorFunc;
    };
} // namespace EMStudio
