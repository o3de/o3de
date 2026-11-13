/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/Component/Component.h>

#include "LUAEditorView.hxx"
#include "LUAEditorStyleMessages.h"

#include <QDialog>


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
        LUAEditorSettingsDialog(QWidget *parent = nullptr);
        ~LUAEditorSettingsDialog() override;
        
        //! Flattened GUI: the element that sizes the labels of the property editor, which the legacy GUI sizes with the sheet the form
        //! gave the editor (LUAEditorSettingsDialog.ui, before the flattening). The first dialog built in the flattened GUI registers
        //! it, once.
        static void RegisterFlatElements();

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
