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

// Description : A dialog bar for quickly accessing menu items and cvars

#ifndef CRYINCLUDE_EDITOR_QUICKACCESSBAR_H
#define CRYINCLUDE_EDITOR_QUICKACCESSBAR_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#endif

class QAction;
class QCompleter;
class QMenuBar;
class QStringListModel;

namespace Ui
{
    class QuickAccessBar;
}

class CQuickAccessBar
    : public QWidget
{
    Q_OBJECT
public:
    CQuickAccessBar(QWidget* pParent = nullptr);
    ~CQuickAccessBar();

    bool eventFilter(QObject* object, QEvent* event) override;

protected:
    void showEvent(QShowEvent* event) override;

    virtual void OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();

    std::map<QString, QAction*> m_menuActionTable;
    int m_lastViewPaneMapVersion;

    void AddMRUFileItems();
private:
    void CollectMenuItems(QMenuBar* menuBar);
    void CollectMenuItems(QAction* action, const QString& path);

    QCompleter* m_completer;
    QStringListModel* m_model;

    QScopedPointer<Ui::QuickAccessBar> m_ui;
    const char* m_levelExtension = nullptr;
};

#endif // CRYINCLUDE_EDITOR_QUICKACCESSBAR_H
