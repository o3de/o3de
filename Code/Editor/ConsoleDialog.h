/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_CONSOLEDIALOG_H
#define CRYINCLUDE_EDITOR_CONSOLEDIALOG_H

#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

class CConsoleSCB;

class CConsoleDialog
    : public QDialog
    , public IInitializeUIInfo
{
    Q_OBJECT
public:
    explicit CConsoleDialog(QWidget* parent = nullptr);
    void SetInfoText(const char* text) override;
    void closeEvent(QCloseEvent*) override;

private:
    CConsoleSCB* const m_consoleWidget;
};

#endif // CRYINCLUDE_EDITOR_CONSOLEDIALOG_H
