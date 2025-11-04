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
        AZ_CLASS_ALLOCATOR(UnsavedChangesDialog, AZ::SystemAllocator);
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
