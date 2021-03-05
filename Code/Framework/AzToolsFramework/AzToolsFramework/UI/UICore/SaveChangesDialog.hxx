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
        AZ_CLASS_ALLOCATOR(SaveChangesDialog, AZ::SystemAllocator, 0);
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
