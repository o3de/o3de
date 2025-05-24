/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>

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
        class AZTF_API NewLogTabDialog
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
