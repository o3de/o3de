/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// ViewPane.h : header file
//

#include "ViewportTitleDlg.h"

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzQtComponents/Components/ToolBarArea.h>
#include <Include/SandboxAPI.h>

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
    CLayoutViewPane(int id, QWidget* parent);
    virtual ~CLayoutViewPane();

    // Operations
public:
    // ActionManagerRegistrationNotificationBus overrides ...
    void OnPostActionManagerRegistrationHook() override;
    void OnMenuLayoutConfig();

    // Set get this pane id.
    int GetId() const { return m_id; }

    void SetViewClass(const QString& sClass);
    QString GetViewClass() const;

    void SetFullscreen(bool f);
    bool IsFullscreen() const { return m_bFullscreen; }

    QWidget* GetViewport() { return m_viewport; }

    //////////////////////////////////////////////////////////////////////////
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

    void closeEvent(QCloseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;


private:
    static constexpr int MIN_VIEWPORT_RES = 64;
    static constexpr int MAX_VIEWPORT_RES = 8192;
    QString m_viewPaneClass;
    bool m_bFullscreen = false;

    const int m_id;

    QWidget* m_viewport = nullptr;
    QScrollArea* m_viewportScrollArea = nullptr;
    ViewportExpansionPolicy m_viewportPolicy = ViewportExpansionPolicy::AutoExpand;

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
