/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#endif

namespace Ui
{
    class NewGraphDialog;
}

namespace ScriptCanvasEditor
{
    class NewGraphDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        NewGraphDialog(const QString& title, const QString& text, QWidget* pParent = nullptr);

        const QString& GetText() const { return m_text; }

    protected:

        void OnOK();
        void OnTextChanged(const QString& text);

        QString m_text;

        Ui::NewGraphDialog* ui;
    };
}
