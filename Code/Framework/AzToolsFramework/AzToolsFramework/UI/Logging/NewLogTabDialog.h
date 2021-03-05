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

#ifndef NEWLOGDIALOG_H
#define NEWLOGDIALOG_H

#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

namespace Ui
{
    class newLogDialog;
}

namespace AzToolsFramework
{
    namespace LogPanel
    {
        //! NewLogTabDialog - the dialog that pops up and asks you to configure a tab in a log control.
        class NewLogTabDialog
            : public QDialog
        {
            Q_OBJECT;
        public:
            NewLogTabDialog(QWidget* pParent = 0);
            ~NewLogTabDialog();

            QString m_windowName;
            QString m_textFilter;
            QString m_tabName;
            bool m_checkError;
            bool m_checkWarning;
            bool m_checkNormal;
            bool m_checkDebug;


        private:
            QScopedPointer<Ui::newLogDialog> uiManager;

        private Q_SLOTS:

            void attemptAccept();
        };
    } // namespace LogPanel
} // namespace AzToolsFramework

#endif