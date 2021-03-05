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

#ifndef CRYINCLUDE_EDITOR_LAYOUTWND_H
#define CRYINCLUDE_EDITOR_LAYOUTWND_H

#pragma once
// LayoutWnd.h : header file
//

#if !defined(Q_MOC_RUN)
#include "Viewport.h"
#include "InfoBar.h"

#include <QSplitter>
#include <QPointer>
#include <AzQtComponents/Components/ToolBarArea.h>
#endif

class CLayoutViewPane;
class CViewport;

class QSettings;

/////////////////////////////////////////////////////////////////////////////
// CLayoutWnd window

enum EViewLayout
{
    ET_Layout0,
    ET_Layout1,
    ET_Layout2,
    ET_Layout3,
    ET_Layout4,
    ET_Layout5,
    ET_Layout6,
    ET_Layout7,
    ET_Layout8
};

#define MAX_VIEWPORTS 9

/** Window used as splitter window in Layout.
*/
class CLayoutSplitter
    : public QSplitter
{
    Q_OBJECT
public:
    CLayoutSplitter(QWidget* parent = nullptr);

    // Implementation
public:
    virtual ~CLayoutSplitter();

    void otherSplitterMoved(int pos, int index) { const QSignalBlocker sb(this);  QSplitter::moveSplitter(pos, index); }

    // Generated message map functions
protected:
    void resizeEvent(QResizeEvent* event) override;

    void CreateLayoutView(int row, int col, int id);

    QSplitterHandle* createHandle() override;

private:
    friend class CLayoutWnd;
};

class InfoBarExpanderWatcher;

/** Main layout window.
*/
class CLayoutWnd
    : public AzQtComponents::ToolBarArea
{
    // Construction
    Q_OBJECT
public:
    CLayoutWnd(QSettings* settings, QWidget* parent = nullptr);

    // Attributes
public:
    CLayoutViewPane* GetViewPane(int id);
    CLayoutViewPane* GetViewPaneByIndex(unsigned int index);
    unsigned int GetViewPaneCount();

    //! Maximize viewport with specified type.
    void MaximizeViewport(int paneId);

    //! Create specific layout.
    void CreateLayout(EViewLayout layout, bool bBindViewports = true, EViewportType defaultView = ET_ViewportCamera);
    EViewLayout GetLayout() const { return m_layout; }

    //! Save layout window configuration to registry.
    void SaveConfig();
    //! Load layout window configuration from registry.
    bool LoadConfig();
    //! Get the config group name in the registry.
    static const char* GetConfigGroupName();

    CLayoutViewPane* FindViewByClass(const QString& viewClassName);
    void BindViewport(CLayoutViewPane* vp, const QString& viewClassName, QWidget* pViewport = NULL);
    QString ViewportTypeToClassName(EViewportType viewType);

    //! Switch 2D viewports.
    void Cycle2DViewport();

    CInfoBar& GetInfoBar() { return *m_infoBar; }

public slots:
    void ResetLayout();

public:
    virtual ~CLayoutWnd();

    // Generated message map functions
protected:
    void OnDestroy();

    // Bind viewports to split panes.
    void BindViewports();
    void UnbindViewports();

    void CreateLayoutView(CLayoutSplitter* wndSplitter, int row, int col, int id, EViewportType viewType);

    bool CycleViewport(EViewportType from, EViewportType to);

private slots:
    void OnFocusChanged(QWidget* oldWidget, QWidget* newWidget);

private:
    void FocusFirstLayoutViewPane(CLayoutSplitter*);
    void MoveViewport(CLayoutViewPane* from, CLayoutViewPane* to, const QString& viewClassName);
    bool m_bMaximized;

    //! What view type is current maximized.
    EViewLayout m_layout;

    // ViewPane id to viewport type
    QString m_viewType[MAX_VIEWPORTS];

    //! Primary split window.
    QPointer<CLayoutSplitter> m_splitWnd;
    //! Secondary split window.
    QPointer<CLayoutSplitter> m_splitWnd2;
    //! Tertiary split window.
    QPointer<CLayoutSplitter> m_splitWnd3;

    //! View pane for maximized layout.
    QPointer<CLayoutViewPane> m_maximizedView;
    // Id of maximized view pane.
    int m_maximizedViewId;

    CInfoBar* m_infoBar;
    QToolBar* m_infoToolBar;
    QSize m_infoBarSize;
    QSettings* m_settings;
    InfoBarExpanderWatcher* m_expanderWatcher;
};

/////////////////////////////////////////////////////////////////////////////

#endif // CRYINCLUDE_EDITOR_LAYOUTWND_H
