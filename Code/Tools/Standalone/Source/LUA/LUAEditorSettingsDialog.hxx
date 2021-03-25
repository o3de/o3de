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
        AZ_CLASS_ALLOCATOR(LUAEditorSettingsDialog, AZ::SystemAllocator, 0);
        LUAEditorSettingsDialog(QWidget *parent = 0);
        ~LUAEditorSettingsDialog() override;
        
    public Q_SLOTS:

        void OnSave();
        void OnSaveClose();
        void OnCancel();

    private:

        void keyPressEvent(QKeyEvent* event) override;

        SyntaxStyleSettings m_originalSettings;
        Ui::LUAEditorSettingsDialog* m_gui;
    };
}