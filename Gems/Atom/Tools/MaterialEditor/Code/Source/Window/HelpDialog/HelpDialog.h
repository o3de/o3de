/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
