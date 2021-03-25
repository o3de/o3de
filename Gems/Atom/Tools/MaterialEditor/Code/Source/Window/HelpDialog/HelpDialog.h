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
#include <AzCore/Memory/SystemAllocator.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QDialog>
#include <Source/Window/HelpDialog/ui_HelpDialog.h>
AZ_POP_DISABLE_WARNING
#endif

namespace Ui
{
    class HelpDialogWidget;
}

namespace MaterialEditor
{
    //! Displays help for Material Editor
    class HelpDialog
        : public QDialog
    {
        Q_OBJECT
    public:
        HelpDialog(QWidget* parent = nullptr);
        ~HelpDialog();

        QScopedPointer<Ui::HelpDialogWidget> m_ui;
    };
} // namespace MaterialEditor
