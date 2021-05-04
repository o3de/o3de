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

#ifndef CRYINCLUDE_EDITOR_VIEWPORTTITLEDLG_H
#define CRYINCLUDE_EDITOR_VIEWPORTTITLEDLG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "RenderViewport.h"
#include <AzCore/Component/Component.h>

#include <functional>
#include <QSharedPointer>
#endif

// CViewportTitleDlg dialog
class CLayoutViewPane;
class CPopupMenuItem;

class QAbstractButton;
class QMenu;

struct ICVar;

namespace Ui
{
    class ViewportTitleDlg;
}

//////////////////////////////////////////////////////////////////////////
class CViewportTitleDlg
    : public QWidget
    , public IEditorNotifyListener
    , public ISystemEventListener
{
    Q_OBJECT
public:
    CViewportTitleDlg(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CViewportTitleDlg();

    void SetViewPane(CLayoutViewPane* pViewPane);
    void SetTitle(const QString& title);
    void OnViewportSizeChanged(int width, int height);
    void OnViewportFOVChanged(float fov);

    static void AddFOVMenus(QMenu* menu, std::function<void(float)> callback, const QStringList& customPresets);
    static void AddAspectRatioMenus(QMenu* menu, std::function<void(int, int)> callback, const QStringList& customPresets);
    static void AddResolutionMenus(QMenu* menu, std::function<void(int, int)> callback, const QStringList& customPresets);

    static void LoadCustomPresets(const QString& section, const QString& keyName, QStringList& outCustompresets);
    static void SaveCustomPresets(const QString& section, const QString& keyName, const QStringList& custompresets);
    static void UpdateCustomPresets(const QString& text, QStringList& custompresets);
    static void OnChangedDisplayInfo(ICVar*    pDisplayInfo, QAbstractButton* pDisplayInfoButton);

    bool eventFilter(QObject* object, QEvent* event) override;

    QMenu* const GetFovMenu();
    QMenu* const GetAspectMenu();
    QMenu* const GetResolutionMenu();

protected:
    virtual void OnInitDialog();

    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);
    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

    void OnMaximize();
    void OnToggleHelpers();
    void OnToggleDisplayInfo();

    QString m_title;

    CLayoutViewPane* m_pViewPane;

    static const int MAX_NUM_CUSTOM_PRESETS = 10;
    QStringList m_customResPresets;
    QStringList m_customFOVPresets;
    QStringList m_customAspectRatioPresets;

    uint64 m_displayInfoCallbackIndex;

    void OnMenuFOVCustom();

    void CreateFOVMenu();
    void PopUpFOVMenu();

    void OnMenuAspectRatioCustom();
    void CreateAspectMenu();
    void PopUpAspectMenu();

    void OnMenuResolutionCustom();
    void CreateResolutionMenu();
    void PopUpResolutionMenu();

    QMenu* m_fovMenu = nullptr;
    QMenu* m_aspectMenu = nullptr;
    QMenu* m_resolutionMenu = nullptr;

    QScopedPointer<Ui::ViewportTitleDlg> m_ui;
};

namespace AzToolsFramework
{
    //! A component to reflect scriptable commands for the Editor
    class ViewportTitleDlgPythonFuncsHandler
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(ViewportTitleDlgPythonFuncsHandler, "{2D686C2D-04F0-4C96-B432-0702E774062E}")

        SANDBOX_API static void Reflect(AZ::ReflectContext* context);

        // AZ::Component ...
        void Activate() override {}
        void Deactivate() override {}
    };

} // namespace AzToolsFramework


#endif // CRYINCLUDE_EDITOR_VIEWPORTTITLEDLG_H
