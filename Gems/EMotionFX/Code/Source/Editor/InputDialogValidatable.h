/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <Editor/LineEditValidatable.h>
#include <QDialog>
#endif

QT_FORWARD_DECLARE_CLASS(QDialogButtonBox)

namespace EMStudio
{
    class InputDialogValidatable
        : public QDialog
    {
        Q_OBJECT //AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR_DECL

        InputDialogValidatable(QWidget* parent, const QString& labelText, const QRegExp regExp = LineEditValidatable::s_defaultRegExp);
        // When destructing clear the Validator which sets the LineEdit Validator to stop lambda validates being called
        ~InputDialogValidatable() { SetValidatorFunc(nullptr); }

        void SetText(const QString& text);
        QString GetText() const;

        void SetValidatorFunc(const AZStd::function<bool()>& func);

    private slots:
        void OnAccepted();

    private:
        LineEditValidatable* m_lineEdit = nullptr;
        QDialogButtonBox* m_buttonBox = nullptr;
    };
} // namespace EMStudio
