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
    void OnViewportFOVChanged(float fovRadians);

    static void AddFOVMenus(QMenu* menu, std::function<void(float)> callback, const QStringList& customPresets);
    static void AddAspectRatioMenus(QMenu* menu, std::function<void(int, int)> callback, const QStringList& customPresets);
    static void AddResolutionMenus(QMenu* menu, std::function<void(int, int)> callback, const QStringList& customPresets);

    static void LoadCustomPresets(const QString& section, const QString& keyName, QStringList& outCustompresets);
    static void SaveCustomPresets(const QString& section, const QString& keyName, const QStringList& custompresets);
    static void UpdateCustomPresets(const QString& text, QStringList& custompresets);

    bool eventFilter(QObject* object, QEvent* event) override;

    QMenu* const GetFovMenu();
    QMenu* const GetAspectMenu();
    QMenu* const GetResolutionMenu();

    void InitializePrefabViewportFocusPathHandler(AzQtComponents::BreadCrumbs* breadcrumbsWidget, QToolButton* backButton);

Q_SIGNALS:
    void ActionTriggered(int command);

protected:
    virtual void OnInitDialog();

    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;
    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

    void OnMaximize();
    void UpdatePrefabEditMode();
    void UpdateDisplayInfo();

    void SetupCameraDropdownMenu();
    void SetupResolutionDropdownMenu();
    void SetupPrefabEditModeMenu();
    void SetupViewportInformationMenu();
    void SetupOverflowMenu();
    void SetupHelpersButton();

    QString m_title;

    CLayoutViewPane* m_pViewPane;

    static const int MAX_NUM_CUSTOM_PRESETS = 10;
    QStringList m_customResPresets;
    QStringList m_customFOVPresets;
    QStringList m_customAspectRatioPresets;

    float m_prevMoveSpeedScale;

    // Speed combobox/lineEdit settings
    double m_speedScaleMin = 0.001;
    double m_speedScaleMax = 100.0;
    double m_speedScaleStep = 0.01;
    int m_speedScaleDecimalCount = 3;

    void OnMenuFOVCustom();

    void CreateFOVMenu();

    void OnMenuAspectRatioCustom();
    void CreateAspectMenu();

    void OnMenuResolutionCustom();
    void CreateResolutionMenu();
    
    void CreatePrefabEditModeMenu();
    QMenu* const GetPrefabEditModeMenu();
    void SetNormalPrefabEditMode();
    void SetMonochromaticPrefabEditMode();

    void CreateViewportInformationMenu();
    QMenu* const GetViewportInformationMenu();
    void SetNoViewportInfo();
    void SetNormalViewportInfo();
    void SetFullViewportInfo();
    void SetCompactViewportInfo();

    void OnBnClickedGotoPosition();
    void OnBnClickedMuteAudio();

    void UpdateMuteActionText();

    void OnToggleDisplayInfo();

    void CheckForCameraSpeedUpdate();

    void OnGridSnappingToggled(int state);
    void OnAngleSnappingToggled(int state);

    void OnGridSpinBoxChanged(double value);
    void OnAngleSpinBoxChanged(double value);

    void UpdateOverFlowMenuState();

    QAction* m_normalPrefabEditModeAction = nullptr;
    QAction* m_monochromaticPrefabEditModeAction = nullptr;
    QMenu* m_fovMenu = nullptr;
    QMenu* m_aspectMenu = nullptr;
    QMenu* m_resolutionMenu = nullptr;
    QMenu* m_prefabEditModeMenu = nullptr;
    QMenu* m_viewportInformationMenu = nullptr;
    QMenu* m_helpersMenu = nullptr;
    QAction* m_helpersAction = nullptr;
    QAction* m_iconsAction = nullptr;
    QAction* m_onlySelectedAction = nullptr;
    QAction* m_noInformationAction = nullptr;
    QAction* m_normalInformationAction = nullptr;
    QAction* m_fullInformationAction = nullptr;
    QAction* m_compactInformationAction = nullptr;
    QAction* m_audioMuteAction = nullptr;
    QCheckBox* m_enableGridSnappingCheckBox = nullptr;
    QCheckBox* m_enableGridVisualizationCheckBox = nullptr;
    QCheckBox* m_enableAngleSnappingCheckBox = nullptr;
    AzQtComponents::DoubleSpinBox* m_cameraSpinBox = nullptr;
    AzQtComponents::DoubleSpinBox* m_gridSpinBox = nullptr;
    AzQtComponents::DoubleSpinBox* m_angleSpinBox = nullptr;
    QWidgetAction* m_gridSizeActionWidget = nullptr;
    QWidgetAction* m_angleSizeActionWidget = nullptr;

    AzToolsFramework::Prefab::PrefabViewportFocusPathHandler* m_prefabViewportFocusPathHandler = nullptr;

    QScopedPointer<Ui::ViewportTitleDlg> m_ui;

    //! The different prefab edit mode effects available in the Edit mode menu.
    enum class PrefabEditModeUXSetting
    {
        Normal, //!< No effect.
        Monochromatic //!< Monochromatic effect.
    };

    //! The currently active edit mode effect.
    PrefabEditModeUXSetting m_prefabEditMode = PrefabEditModeUXSetting::Monochromatic;
};

namespace AzToolsFramework
{
    //! A component to reflect scriptable commands for the Editor.
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
