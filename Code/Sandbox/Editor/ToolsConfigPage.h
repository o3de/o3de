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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_TOOLSCONFIGPAGE_H
#define CRYINCLUDE_EDITOR_TOOLSCONFIGPAGE_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <QStringListModel>
#endif

namespace Ui
{
    class IconListDialog;
    class ToolsConfigPage;
}

class CommandModel;
class MacroModel;

class QStringListModel;

class CIconListDialog
    : public QDialog
{
    Q_OBJECT
public:
    CIconListDialog(QWidget* pParent = nullptr);

    bool GetSelectedIconPath(QString& path) const;

protected:
    void OnInitDialog();

private:
    QScopedPointer<Ui::IconListDialog> m_ui;
};

class ToolsConfigDialog
    : public QDialog
{
    Q_OBJECT
public:
    ToolsConfigDialog(QWidget* parent = nullptr);

private:
    void reject() override;
    void closeEvent(QCloseEvent* ev) override;
};

/** Tools configuration property page.
*/
class CToolsConfigPage
    : public QWidget
{
    Q_OBJECT
public:
    CToolsConfigPage(QWidget* parent = nullptr);
    virtual ~CToolsConfigPage();

    virtual void OnOK();
    virtual void OnCancel();

protected:
    virtual void OnInitDialog();

    void OnSelchangeMacroList();
    void OnSelchangeCommandList();

    void OnAssignCommand();
    void OnAssignMacroShortcut();
    void OnSelectMacroIcon();
    void OnClearMacroIcon();

    void OnConsoleCmd();
    void OnScriptCmd();

    void OnNewMacroItem();
    void OnNewCommandItem();

    void OnMoveMacroItemUp();
    void OnMoveMacroItemDown();
    void OnMoveCommandItemUp();
    void OnMoveCommandItemDown();

    void OnDeleteMacroItem();
    void OnDeleteCommandItem();

    //////////////////////////////////////////////////////////////////////////
    // Vars.
    //////////////////////////////////////////////////////////////////////////
    int m_consoleOrScript;  // 0 -> console commands, 1 -> script commands

    void FillConsoleCmds();
    void FillScriptCmds();

private:
    MacroModel* m_macroModel;
    CommandModel* m_commandModel;
    QStringListModel* m_completionModel;

    QScopedPointer<Ui::ToolsConfigPage> m_ui;
};

#endif // CRYINCLUDE_EDITOR_TOOLSCONFIGPAGE_H
