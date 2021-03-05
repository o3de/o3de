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
        AZ_CLASS_ALLOCATOR(OverwritePromptDialog, AZ::SystemAllocator, 0);
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
