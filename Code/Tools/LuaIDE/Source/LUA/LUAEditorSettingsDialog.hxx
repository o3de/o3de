/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/Component/Component.h>
#include "LUAEditorView.hxx"
#include "LUAEditorStyleMessages.h"

#include <QtWidgets/QDialog>
#endif

#pragma once

namespace Ui
{
    class LUAEditorSettingsDialog;
}

namespace LUAEditor
{
    class LUAEditorSettingsDialog 
        : public QDialog
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(LUAEditorSettingsDialog, AZ::SystemAllocator);
        LUAEditorSettingsDialog(QWidget *parent = 0);
        ~LUAEditorSettingsDialog() override;
        
    public Q_SLOTS:

        void OnSave();
        void OnSaveClose();
        void OnCancel();
        void OnApply();

    private:

        void keyPressEvent(QKeyEvent* event) override;

        SyntaxStyleSettings m_originalSettings;
        Ui::LUAEditorSettingsDialog* m_gui;
    };
}
