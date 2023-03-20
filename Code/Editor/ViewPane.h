/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_VIEWPANE_H
#define CRYINCLUDE_EDITOR_VIEWPANE_H

#pragma once
// ViewPane.h : header file
//

#if !defined(Q_MOC_RUN)
#include "ViewportTitleDlg.h"

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzQtComponents/Components/ToolBarArea.h>
#include <Include/SandboxAPI.h>
#endif

class CViewport;
class QScrollArea;
class QToolBar;
class ViewportTitleExpanderWatcher;

namespace AzToolsFramework
{
    class ActionManagerInterface;
    class MenuManagerInterface;
    class ToolBarManagerInterface;
}

/////////////////////////////////////////////////////////////////////////////
// CViewPane view

class CLayoutViewPane
    : public AzQtComponents::ToolBarArea
    , private AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
{
    Q_OBJECT
public:
    CLayoutViewPane(QWidget* parent = nullptr);
    virtual ~CLayoutViewPane();

    // Operations
public:
    // ActionManagerRegistrationNotificationBus overrides ...
    void OnMenuRegistrationHook() override;
    void OnToolBarRegistrationHook() override;
    void OnActionRegistrationHook() override;
    void OnMenuBindingHook() override;
    void OnToolBarBindingHook() override;

    // Set get this pane id.
    void SetId(int id) { m_id = id; }
    int GetId() { return m_id; }

    void SetViewClass(const QString& sClass);
    QString GetViewClass() const;

    //! Assign viewport to this pane.
    void SwapViewports(CLayoutViewPane* pView);

    void AttachViewport(QWidget* pViewport);
    //! Detach viewport from this pane.
    void DetachViewport();
    // Releases and destroy attached viewport.
    void ReleaseViewport();

    void SetFullscren(bool f);
    bool IsFullscreen() const { return m_bFullscreen; }

    QWidget* GetViewport() { return m_viewport; }

    //////////////////////////////////////////////////////////////////////////
    bool IsActiveView() const { return m_active; }

    void ShowTitleMenu();
    void ToggleMaximize();

    void SetFocusToViewport();

    void ResizeViewport(int width, int height);
    void SetAspectRatio(unsigned int x, unsigned int y);
    void SetViewportFOV(float fovDegrees);
    void OnFOVChanged(float fovRadians);

    enum class ViewportExpansionPolicy
    {
        AutoExpand,
        FixedSize
    };

    ViewportExpansionPolicy GetViewportExpansionPolicy() { return m_viewportPolicy; }

    //! Determine whether the viewport will be a fixed-size (scrollable if necessary) 
    //! independent of the main window size, or auto-expanding based directly on the main window size.
    void SetViewportExpansionPolicy(ViewportExpansionPolicy policy);

protected:
    void OnMenuLayoutConfig();
    void OnMenuViewSelected(const QString& paneName);
    void OnMenuMaximized();

    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void OnDestroy();
    void focusInEvent(QFocusEvent* event) override;

    void DisconnectRenderViewportInteractionRequestBus();

private:
    static constexpr int MIN_VIEWPORT_RES = 64;
    static constexpr int MAX_VIEWPORT_RES = 8192;
    static constexpr int MAX_CLASSVIEWS = 100;
    static constexpr int VIEW_BORDER = 0;

    enum TitleMenuCommonCommands
    {
        ID_MAXIMIZED = 50000,
        ID_LAYOUT_CONFIG,

        FIRST_ID_CLASSVIEW,
        LAST_ID_CLASSVIEW = FIRST_ID_CLASSVIEW + MAX_CLASSVIEWS - 1
    };

    QString m_viewPaneClass;
    bool m_bFullscreen;

    CViewportTitleDlg* m_viewportTitleDlg;
    ViewportTitleExpanderWatcher* m_expanderWatcher;

    int m_id;
    int m_nBorder;

    QWidget* m_viewport;
    QScrollArea* m_viewportScrollArea = nullptr;
    ViewportExpansionPolicy m_viewportPolicy = ViewportExpansionPolicy::AutoExpand;
    bool m_active;

    AzToolsFramework::ActionManagerInterface* m_actionManagerInterface = nullptr;
    AzToolsFramework::MenuManagerInterface* m_menuManagerInterface = nullptr;
    AzToolsFramework::ToolBarManagerInterface* m_toolBarManagerInterface = nullptr;
};

/////////////////////////////////////////////////////////////////////////////

namespace AzToolsFramework
{
    //! A component to reflect scriptable commands for the Editor
    class ViewPanePythonFuncsHandler
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(ViewPanePythonFuncsHandler, "{25C99C8F-4440-4656-ABC4-32134F496CC1}")

        SANDBOX_API static void Reflect(AZ::ReflectContext* context);

        // AZ::Component ...
        void Activate() override {}
        void Deactivate() override {}
    };

} // namespace AzToolsFramework

#endif // CRYINCLUDE_EDITOR_VIEWPANE_H
