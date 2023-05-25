/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef SAVECHANGESDIALOG_HXX
#define SAVECHANGESDIALOG_HXX

#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <QObject>

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/LegacyFramework/UIFrameworkAPI.h>
#endif

namespace Ui
{
    class SaveChangesDialog;
}

#pragma once

namespace AzToolsFramework
{
    class SaveChangesDialog
        : public QDialog
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(SaveChangesDialog, AZ::SystemAllocator);
        SaveChangesDialog(QWidget* pParent);
        virtual ~SaveChangesDialog();

    private slots:
        void OnSaveButton();
        void OnCancelButton();
        void OnContinueButton();

    public:
        SaveChangesDialogResult m_result;

        Ui::SaveChangesDialog* guiConstructor;
    };
}

#endif//SAVECHANGESDIALOG_HXX
