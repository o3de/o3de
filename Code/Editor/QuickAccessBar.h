/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
