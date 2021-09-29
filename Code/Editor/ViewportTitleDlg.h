/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_VIEWPORTTITLEDLG_H
#define CRYINCLUDE_EDITOR_VIEWPORTTITLEDLG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Component/Component.h>

#include <functional>
#include <QSharedPointer>
#include <QWidgetAction>
#include <QComboBox>

#include <AzToolsFramework/UI/Prefab/PrefabViewportFocusPathHandler.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>

#include <HMDBus.h>
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
    , public AZ::VR::VREventBus::Handler
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

    bool eventFilter(QObject* object, QEvent* event) override;

    void SetSpeedComboBox(double value);

    QMenu* const GetFovMenu();
    QMenu* const GetAspectMenu();
    QMenu* const GetResolutionMenu();

Q_SIGNALS:
    void ActionTriggered(int command);

protected:
    virtual void OnInitDialog();

    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;
    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

    void OnMaximize();
    void OnToggleHelpers();
    void UpdateDisplayInfo();

    //////////////////////////////////////////////////////////////////////////
    /// VR Event Bus Implementation
    //////////////////////////////////////////////////////////////////////////
    void OnHMDInitialized() override;
    void OnHMDShutdown() override;
    //////////////////////////////////////////////////////////////////////////

    void SetupCameraDropdownMenu();
    void SetupResolutionDropdownMenu();
    void SetupViewportInformationMenu();
    void SetupOverflowMenu();
    void SetupHelpersButton();

    QString m_title;

    CLayoutViewPane* m_pViewPane;

    static const int MAX_NUM_CUSTOM_PRESETS = 10;
    QStringList m_customResPresets;
    QStringList m_customFOVPresets;
    QStringList m_customAspectRatioPresets;

    float m_prevMoveSpeed;

    // Speed combobox/lineEdit settings
    double m_minSpeed = 0.01;
    double m_maxSpeed = 100.0;
    double m_speedStep = 0.001;
    int m_numDecimals = 3;

    // Speed presets
    float m_speedPresetValues[4] = { 0.01f, 0.1f, 1.0f, 10.0f };

    double m_fieldWidthMultiplier = 1.8;


    void OnMenuFOVCustom();

    void CreateFOVMenu();

    void OnMenuAspectRatioCustom();
    void CreateAspectMenu();

    void OnMenuResolutionCustom();
    void CreateResolutionMenu();

    void CreateViewportInformationMenu();
    QMenu* const GetViewportInformationMenu();
    void SetNoViewportInfo();
    void SetNormalViewportInfo();
    void SetFullViewportInfo();
    void SetCompactViewportInfo();

    void OnBnClickedGotoPosition();
    void OnBnClickedMuteAudio();
    void OnBnClickedEnableVR();

    void UpdateMuteActionText();

    void OnToggleDisplayInfo();

    void OnSpeedComboBoxEnter();
    void OnUpdateMoveSpeedText(const QString&);

    void CheckForCameraSpeedUpdate();

    void OnGridSnappingToggled();
    void OnAngleSnappingToggled();

    void OnGridSpinBoxChanged(double value);
    void OnAngleSpinBoxChanged(double value);

    void UpdateOverFlowMenuState();

    QMenu* m_fovMenu = nullptr;
    QMenu* m_aspectMenu = nullptr;
    QMenu* m_resolutionMenu = nullptr;
    QMenu* m_viewportInformationMenu = nullptr;
    QAction* m_noInformationAction = nullptr;
    QAction* m_normalInformationAction = nullptr;
    QAction* m_fullInformationAction = nullptr;
    QAction* m_compactInformationAction = nullptr;
    QAction* m_audioMuteAction = nullptr;
    QAction* m_enableVRAction = nullptr;
    QAction* m_enableGridSnappingAction = nullptr;
    QAction* m_enableAngleSnappingAction = nullptr;
    QComboBox* m_cameraSpeed = nullptr;
    AzQtComponents::DoubleSpinBox* m_gridSpinBox = nullptr;
    AzQtComponents::DoubleSpinBox* m_angleSpinBox = nullptr;
    QWidgetAction* m_gridSizeActionWidget = nullptr;
    QWidgetAction* m_angleSizeActionWidget = nullptr;

    AzToolsFramework::Prefab::PrefabViewportFocusPathHandler* m_prefabViewportFocusPathHandler = nullptr;

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
