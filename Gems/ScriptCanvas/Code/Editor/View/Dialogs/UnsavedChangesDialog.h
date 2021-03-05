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
#include <QDialog>
#endif

namespace Ui
{
    class UnsavedChangesDialog;
}

namespace ScriptCanvasEditor
{
    enum class UnsavedChangesOptions
    {
        SAVE,
        CONTINUE_WITHOUT_SAVING,
        CANCEL_WITHOUT_SAVING,
        INVALID
    };
    
    class UnsavedChangesDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(UnsavedChangesDialog, AZ::SystemAllocator, 0);
        UnsavedChangesDialog(const QString& filename, QWidget* pParent = nullptr);
        UnsavedChangesOptions GetResult() const { return m_result; }

    protected:

        void OnSaveButton(bool clicked);
        void OnContinueWithoutSavingButton(bool clicked);
        void OnCancelWithoutSavingButton(bool clicked);

        Ui::UnsavedChangesDialog* ui;
        UnsavedChangesOptions m_result = UnsavedChangesOptions::INVALID;
    };
}