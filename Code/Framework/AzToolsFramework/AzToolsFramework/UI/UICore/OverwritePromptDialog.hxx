/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef OVERWRITEPROMPTDIALOG_HXX
#define OVERWRITEPROMPTDIALOG_HXX

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/LegacyFramework/UIFrameworkAPI.h>
#include <QDialog>
#endif

namespace Ui
{
    class OverwritePromptDialog;
}

#pragma once

namespace AzToolsFramework
{
    class OverwritePromptDialog
        : public QDialog
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(OverwritePromptDialog, AZ::SystemAllocator);
        OverwritePromptDialog(QWidget* pParent);
        virtual ~OverwritePromptDialog();

        void UpdateLabel(QString label);

    private slots:
        void on_OverwriteButton_clicked();
        void on_CancelButton_clicked();

    public:
        bool m_result;

        Ui::OverwritePromptDialog* guiConstructor;
    };
}

#endif//SAVECHANGESDIALOG_HXX
