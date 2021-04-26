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
