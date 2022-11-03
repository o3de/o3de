/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Description : CViewportTitleDlg implementation file

#if !defined(Q_MOC_RUN)
#include "EditorDefs.h"

#include "ViewportTitleDlg.h"

// Qt
#include <QCheckBox>
#include <QLabel>
#include <QInputDialog>

#include <AtomLyIntegration/AtomViewportDisplayInfo/AtomViewportInfoDisplayBus.h>

// Editor
#include "ActionManager.h"
#include "CustomAspectRatioDlg.h"
#include "CustomResolutionDlg.h"
#include "DisplaySettings.h"
#include "EditorViewportSettings.h"
#include "GameEngine.h"
#include "Include/IObjectManager.h"
#include "MainWindow.h"
#include "MathConversion.h"
#include "Objects/SelectionGroup.h"
#include "Settings.h"
#include "UsedResources.h"
#include "ViewPane.h"

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/algorithm.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInternalInterface.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelectionRequestBus.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <Editor/EditorSettingsAPIBus.h>
#include <EditorModeFeedback/EditorStateRequestsBus.h>

#include <LmbrCentral/Audio/AudioSystemComponentBus.h>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include "ui_ViewportTitleDlg.h"
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
#endif //! defined(Q_MOC_RUN)

static constexpr int MiniumOverflowMenuWidth = 200;

// CViewportTitleDlg dialog

namespace
{
    class CViewportTitleDlgDisplayInfoHelper
        : public QObject
        , public AZ::AtomBridge::AtomViewportInfoDisplayNotificationBus::Handler
    {
        Q_OBJECT

    public:
        CViewportTitleDlgDisplayInfoHelper(CViewportTitleDlg* parent)
            : QObject(parent)
        {
            AZ::AtomBridge::AtomViewportInfoDisplayNotificationBus::Handler::BusConnect();
        }

    signals:
        void ViewportInfoStatusUpdated(int newIndex);

    private:
        void OnViewportInfoDisplayStateChanged(AZ::AtomBridge::ViewportInfoDisplayState state) override
        {
            emit ViewportInfoStatusUpdated(aznumeric_cast<int>(state));
        }
    };

    // simple override of QMenu that does not respond to keyboard events
    // note: this prevents the menu from being prematurely closed
    class IgnoreKeyboardMenu : public QMenu
    {
    public:
        IgnoreKeyboardMenu(QWidget* parent = nullptr)
            : QMenu(parent)
        {
        }

    private:
        void keyPressEvent(QKeyEvent* event) override
        {
            // regular escape key handling
            if (event->key() == Qt::Key_Escape)
            {
                QMenu::keyPressEvent(event);
            }
        }
    };
} // end anonymous namespace

CViewportTitleDlg::CViewportTitleDlg(QWidget* pParent)
    : QWidget(pParent)
    , m_ui(new Ui::ViewportTitleDlg)
{
    auto container = new QWidget(this);
    m_ui->setupUi(container);
    auto layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(container);
    container->setObjectName("ViewportTitleDlgContainer");

    m_prevMoveSpeedScale = 0;

    m_pViewPane = nullptr;
    GetIEditor()->RegisterNotifyListener(this);
    GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);

    LoadCustomPresets("FOVPresets", "FOVPreset", m_customFOVPresets);
    LoadCustomPresets("AspectRatioPresets", "AspectRatioPreset", m_customAspectRatioPresets);
    LoadCustomPresets("ResPresets", "ResPreset", m_customResPresets);

    SetupPrefabEditModeMenu();
    SetupCameraDropdownMenu();
    SetupResolutionDropdownMenu();
    SetupViewportInformationMenu();

    if (!AzToolsFramework::IsNewActionManagerEnabled())
    {
        SetupHelpersButton();
    }
    else
    {
        // Temporary Helpers menu setup until this toolbar is refactored.
        auto menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
        auto menuManagerInternalInterface = AZ::Interface<AzToolsFramework::MenuManagerInternalInterface>::Get();
        if (menuManagerInterface && menuManagerInternalInterface)
        {
            const AZStd::string helpersMenuIdentifier = "o3de.menu.viewport.helpers";
            AzToolsFramework::MenuProperties menuProperties;
            menuProperties.m_name = "Helpers State";
            menuManagerInterface->RegisterMenu(helpersMenuIdentifier, menuProperties);

            QMenu* helpersMenu = menuManagerInternalInterface->GetMenu(helpersMenuIdentifier);
            m_ui->m_helpers->setMenu(helpersMenu);
            m_ui->m_helpers->setPopupMode(QToolButton::InstantPopup);
        }
    }

    SetupOverflowMenu();

    if (gSettings.bMuteAudio)
    {
        LmbrCentral::AudioSystemComponentRequestBus::Broadcast(&LmbrCentral::AudioSystemComponentRequestBus::Events::GlobalMuteAudio);
    }
    else
    {
        LmbrCentral::AudioSystemComponentRequestBus::Broadcast(&LmbrCentral::AudioSystemComponentRequestBus::Events::GlobalUnmuteAudio);
    }

    connect(this, &CViewportTitleDlg::ActionTriggered, MainWindow::instance()->GetActionManager(), &ActionManager::ActionTriggered);

    OnInitDialog();
}

CViewportTitleDlg::~CViewportTitleDlg()
{
    GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);
    GetIEditor()->UnregisterNotifyListener(this);

    if (m_prefabViewportFocusPathHandler)
    {
        delete m_prefabViewportFocusPathHandler;
    }
}

void CViewportTitleDlg::SetupCameraDropdownMenu()
{
    // Setup the camera dropdown menu
    auto cameraMenu = new IgnoreKeyboardMenu(this);
    cameraMenu->addMenu(GetFovMenu());
    m_ui->m_cameraMenu->setMenu(cameraMenu);
    m_ui->m_cameraMenu->setPopupMode(QToolButton::InstantPopup);
    QObject::connect(cameraMenu, &QMenu::aboutToShow, this, &CViewportTitleDlg::CheckForCameraSpeedUpdate);
    QObject::connect(cameraMenu, &QMenu::aboutToHide, &QWidget::clearFocus);

    QAction* gotoPositionAction = new QAction("Go to position", cameraMenu);
    connect(gotoPositionAction, &QAction::triggered, this, &CViewportTitleDlg::OnBnClickedGotoPosition);
    cameraMenu->addAction(gotoPositionAction);

    cameraMenu->addSeparator();

    auto cameraSpeedActionWidget = new QWidgetAction(cameraMenu);
    auto cameraSpeedContainer = new QWidget(cameraMenu);
    auto cameraSpeedLabel = new QLabel(tr("Camera Speed Scale"), cameraMenu);
    m_cameraSpinBox = new AzQtComponents::DoubleSpinBox();
    m_cameraSpinBox->setRange(m_speedScaleMin, m_speedScaleMax);
    m_cameraSpinBox->SetDisplayDecimals(m_speedScaleDecimalCount);
    m_cameraSpinBox->setSingleStep(m_speedScaleStep);
    m_cameraSpinBox->setValue(SandboxEditor::CameraSpeedScale());

    QObject::connect(m_cameraSpinBox, &AzQtComponents::DoubleSpinBox::editingFinished, &QWidget::clearFocus);
    QObject::connect(
        m_cameraSpinBox,
        QOverload<double>::of(&AzQtComponents::DoubleSpinBox::valueChanged),
        [](const double value)
        {
            SandboxEditor::SetCameraSpeedScale(aznumeric_cast<float>(value));
        });

    // the padding to give to the layout to align with the QAction entries in the list
    const int CameraSpeedLayoutPadding = 16;
    QHBoxLayout* cameraSpeedLayout = new QHBoxLayout;
    cameraSpeedLayout->setContentsMargins(QMargins(CameraSpeedLayoutPadding, 0, CameraSpeedLayoutPadding, 0));
    cameraSpeedLayout->addWidget(cameraSpeedLabel);
    cameraSpeedLayout->addWidget(m_cameraSpinBox);
    cameraSpeedContainer->setLayout(cameraSpeedLayout);
    cameraSpeedActionWidget->setDefaultWidget(cameraSpeedContainer);

    cameraMenu->addAction(cameraSpeedActionWidget);
}

void CViewportTitleDlg::SetupResolutionDropdownMenu()
{
    // Setup the resolution dropdown menu
    QMenu* resolutionMenu = new QMenu(this);
    resolutionMenu->addMenu(GetAspectMenu());
    resolutionMenu->addMenu(GetResolutionMenu());
    m_ui->m_resolutionMenu->setMenu(resolutionMenu);
    m_ui->m_resolutionMenu->setPopupMode(QToolButton::InstantPopup);
}

void CViewportTitleDlg::SetupPrefabEditModeMenu()
{
    m_prefabEditMode = AzToolsFramework::PrefabEditModeEffectEnabled() ? PrefabEditModeUXSetting::Monochromatic : PrefabEditModeUXSetting::Normal;
    m_ui->m_prefabEditModeMenu->setMenu(GetPrefabEditModeMenu());
    m_ui->m_prefabEditModeMenu->setAutoRaise(true);
    m_ui->m_prefabEditModeMenu->setPopupMode(QToolButton::InstantPopup);
    UpdatePrefabEditMode();
}

void CViewportTitleDlg::SetupViewportInformationMenu()
{
    // Setup the debug information button
    m_ui->m_debugInformationMenu->setMenu(GetViewportInformationMenu());
    connect(m_ui->m_debugInformationMenu, &QToolButton::clicked, this, &CViewportTitleDlg::OnToggleDisplayInfo);
    m_ui->m_debugInformationMenu->setPopupMode(QToolButton::MenuButtonPopup);
}

static bool ShouldHelpersBeChecked()
{
    return AzToolsFramework::HelpersVisible() || AzToolsFramework::IconsVisible() || AzToolsFramework::OnlyShowHelpersForSelectedEntities();
}

void CViewportTitleDlg::SetupHelpersButton()
{
    if (m_helpersMenu == nullptr)
    {
        m_helpersMenu = new QMenu("Helpers State", this);

        auto helperAction = MainWindow::instance()->GetActionManager()->GetAction(AzToolsFramework::Helpers);
        connect(
            helperAction, &QAction::triggered, this,
            [this]
            {
                m_ui->m_helpers->setChecked(ShouldHelpersBeChecked());
            });

        auto iconAction = MainWindow::instance()->GetActionManager()->GetAction(AzToolsFramework::Icons);
        connect(
            iconAction,
            &QAction::triggered,
            this,
            [this]
            {
                m_ui->m_helpers->setChecked(ShouldHelpersBeChecked());
            });

        auto onlySelectedAction = MainWindow::instance()->GetActionManager()->GetAction(AzToolsFramework::OnlyShowHelpersForSelectedEntitiesAction);
        connect(
            onlySelectedAction,
            &QAction::triggered,
            this,
            [this]
            {
                m_ui->m_helpers->setChecked(ShouldHelpersBeChecked());
            });

        m_helpersAction = new QAction(tr("Helpers"), m_helpersMenu);
        m_helpersAction->setCheckable(true);
        connect(
            m_helpersAction, &QAction::triggered, this,
            [helperAction]
            {
                helperAction->trigger();
            });

        m_iconsAction = new QAction(tr("Icons"), m_helpersMenu);
        m_iconsAction->setCheckable(true);
        connect(
            m_iconsAction, &QAction::triggered, this,
            [iconAction]
            {
                iconAction->trigger();
            });

        m_onlySelectedAction = new QAction(tr("Helpers for Selected Entities Only"), m_helpersMenu);
        m_onlySelectedAction->setCheckable(true);
        connect(
             m_onlySelectedAction,
            &QAction::triggered,
            this,
            [onlySelectedAction]
            {
                onlySelectedAction->trigger();
            });

        m_helpersMenu->addAction(m_helpersAction);
        m_helpersMenu->addAction(m_iconsAction);
        m_helpersMenu->addAction(m_onlySelectedAction);

        connect(
            m_helpersMenu, &QMenu::aboutToShow, this,
            [this]
            {
                m_helpersAction->setChecked(AzToolsFramework::HelpersVisible());
                m_iconsAction->setChecked(AzToolsFramework::IconsVisible());
                m_onlySelectedAction->setChecked(AzToolsFramework::OnlyShowHelpersForSelectedEntities());
            });

        m_ui->m_helpers->setCheckable(true);
        m_ui->m_helpers->setMenu(m_helpersMenu);
        m_ui->m_helpers->setPopupMode(QToolButton::InstantPopup);
    }

    m_ui->m_helpers->setChecked(ShouldHelpersBeChecked());
}

void CViewportTitleDlg::SetupOverflowMenu()
{
    // setup the overflow menu
    auto* overflowMenu = new IgnoreKeyboardMenu(this);
    overflowMenu->setMinimumWidth(MiniumOverflowMenuWidth);

    m_audioMuteAction = new QAction("Mute Audio", overflowMenu);
    connect(m_audioMuteAction, &QAction::triggered, this, &CViewportTitleDlg::OnBnClickedMuteAudio);
    overflowMenu->addAction(m_audioMuteAction);

    overflowMenu->addSeparator();

    m_enableGridSnappingCheckBox = new QCheckBox("Enable Grid Snapping", overflowMenu);
    AzQtComponents::CheckBox::applyToggleSwitchStyle(m_enableGridSnappingCheckBox);
    auto gridSnappingWidgetAction = new QWidgetAction(overflowMenu);
    gridSnappingWidgetAction->setDefaultWidget(m_enableGridSnappingCheckBox);
    connect(m_enableGridSnappingCheckBox, &QCheckBox::stateChanged, this, &CViewportTitleDlg::OnGridSnappingToggled);
    overflowMenu->addAction(gridSnappingWidgetAction);

    m_enableGridVisualizationCheckBox = new QCheckBox("Show Grid", overflowMenu);
    AzQtComponents::CheckBox::applyToggleSwitchStyle(m_enableGridVisualizationCheckBox);
    auto gridVisualizationWidgetAction = new QWidgetAction(overflowMenu);
    gridVisualizationWidgetAction->setDefaultWidget(m_enableGridVisualizationCheckBox);
    connect(
        m_enableGridVisualizationCheckBox, &QCheckBox::stateChanged,
        [](const int state)
        {
            SandboxEditor::SetShowingGrid(state == Qt::Checked);
        });
    overflowMenu->addAction(gridVisualizationWidgetAction);

    m_gridSizeActionWidget = new QWidgetAction(overflowMenu);
    m_gridSpinBox = new AzQtComponents::DoubleSpinBox();
    m_gridSpinBox->setValue(SandboxEditor::GridSnappingSize());
    m_gridSpinBox->setMinimum(1e-2f);
    m_gridSpinBox->setToolTip(tr("Grid size"));

    QObject::connect(
        m_gridSpinBox, QOverload<double>::of(&AzQtComponents::DoubleSpinBox::valueChanged), this, &CViewportTitleDlg::OnGridSpinBoxChanged);

    m_gridSizeActionWidget->setDefaultWidget(m_gridSpinBox);
    overflowMenu->addAction(m_gridSizeActionWidget);

    overflowMenu->addSeparator();

    m_enableAngleSnappingCheckBox = new QCheckBox("Enable Angle Snapping", overflowMenu);
    AzQtComponents::CheckBox::applyToggleSwitchStyle(m_enableAngleSnappingCheckBox);
    auto angleSnappingWidgetAction = new QWidgetAction(overflowMenu);
    angleSnappingWidgetAction->setDefaultWidget(m_enableAngleSnappingCheckBox);
    connect(m_enableAngleSnappingCheckBox, &QCheckBox::stateChanged, this, &CViewportTitleDlg::OnAngleSnappingToggled);
    overflowMenu->addAction(angleSnappingWidgetAction);

    m_angleSizeActionWidget = new QWidgetAction(overflowMenu);
    m_angleSpinBox = new AzQtComponents::DoubleSpinBox();
    m_angleSpinBox->setValue(SandboxEditor::AngleSnappingSize());
    m_angleSpinBox->setMinimum(1e-2f);
    m_angleSpinBox->setToolTip(tr("Angle size"));

    QObject::connect(
        m_angleSpinBox, QOverload<double>::of(&AzQtComponents::DoubleSpinBox::valueChanged), this,
        &CViewportTitleDlg::OnAngleSpinBoxChanged);

    m_angleSizeActionWidget->setDefaultWidget(m_angleSpinBox);
    overflowMenu->addAction(m_angleSizeActionWidget);

    m_ui->m_overflowBtn->setMenu(overflowMenu);
    m_ui->m_overflowBtn->setPopupMode(QToolButton::InstantPopup);
    connect(overflowMenu, &QMenu::aboutToShow, this, &CViewportTitleDlg::UpdateOverFlowMenuState);

    UpdateMuteActionText();
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::SetViewPane(CLayoutViewPane* pViewPane)
{
    if (m_pViewPane)
    {
        m_pViewPane->disconnect(this);
    }

    m_pViewPane = pViewPane;

    if (m_pViewPane)
    {
        connect(this, &QWidget::customContextMenuRequested, m_pViewPane, &CLayoutViewPane::ShowTitleMenu);
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnInitDialog()
{
    // Add a child parented to us that listens for r_displayInfo changes.
    auto displayInfoHelper = new CViewportTitleDlgDisplayInfoHelper(this);
    connect(displayInfoHelper, &CViewportTitleDlgDisplayInfoHelper::ViewportInfoStatusUpdated, this, &CViewportTitleDlg::UpdateDisplayInfo);
    UpdateDisplayInfo();

    bool isPrefabSystemEnabled = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(isPrefabSystemEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);

    if (!isPrefabSystemEnabled)
    {
        m_ui->m_prefabFocusPath->setEnabled(false);
        m_ui->m_prefabFocusBackButton->setEnabled(false);
        m_ui->m_prefabFocusPath->hide();
        m_ui->m_prefabFocusBackButton->hide();
    }
}

void CViewportTitleDlg::InitializePrefabViewportFocusPathHandler(AzQtComponents::BreadCrumbs* breadcrumbsWidget, QToolButton* backButton)
{
    if (m_prefabViewportFocusPathHandler != nullptr)
    {
        return;
    }

    bool isPrefabSystemEnabled = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(isPrefabSystemEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);

    if (isPrefabSystemEnabled)
    {
        m_prefabViewportFocusPathHandler = new AzToolsFramework::Prefab::PrefabViewportFocusPathHandler();
        m_prefabViewportFocusPathHandler->Initialize(breadcrumbsWidget, backButton);
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::SetTitle(const QString& title)
{
    m_title = title;
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnMaximize()
{
    if (m_pViewPane)
    {
        m_pViewPane->ToggleMaximize();
    }
}

void CViewportTitleDlg::SetNormalPrefabEditMode()
{
    m_prefabEditMode = PrefabEditModeUXSetting::Normal;
    UpdatePrefabEditMode();
}

void CViewportTitleDlg::SetMonochromaticPrefabEditMode()
{
    m_prefabEditMode = PrefabEditModeUXSetting::Monochromatic;
    UpdatePrefabEditMode();
}

void CViewportTitleDlg::SetNoViewportInfo()
{
    AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Broadcast(
        &AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Events::SetDisplayState, AZ::AtomBridge::ViewportInfoDisplayState::NoInfo);
}

void CViewportTitleDlg::SetNormalViewportInfo()
{
    AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Broadcast(
        &AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Events::SetDisplayState, AZ::AtomBridge::ViewportInfoDisplayState::NormalInfo);
}

void CViewportTitleDlg::SetFullViewportInfo()
{
    AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Broadcast(
        &AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Events::SetDisplayState, AZ::AtomBridge::ViewportInfoDisplayState::FullInfo);
}

void CViewportTitleDlg::SetCompactViewportInfo()
{
    AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Broadcast(
        &AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Events::SetDisplayState, AZ::AtomBridge::ViewportInfoDisplayState::CompactInfo);
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::UpdatePrefabEditMode()
{
    if (m_prefabEditModeMenu == nullptr)
    {
        // Nothing to update, just return;
        return;
    }

    m_normalPrefabEditModeAction->setChecked(false);
    m_monochromaticPrefabEditModeAction->setChecked(false);

    switch (m_prefabEditMode)
    {
    case PrefabEditModeUXSetting::Normal:
        {
            m_normalPrefabEditModeAction->setChecked(true);
            AZ::Render::EditorStateRequestsBus::Event(
                AZ::Render::EditorState::FocusMode, &AZ::Render::EditorStateRequestsBus::Events::SetEnabled, false);
            AzToolsFramework::SetPrefabEditModeEffectEnabled(false);
            AzToolsFramework::EditorSettingsAPIBus::Broadcast(&AzToolsFramework::EditorSettingsAPIBus::Events::SaveSettingsRegistryFile);
            break;
        }
    case PrefabEditModeUXSetting::Monochromatic:
        {
            m_monochromaticPrefabEditModeAction->setChecked(true);
            AZ::Render::EditorStateRequestsBus::Event(
                AZ::Render::EditorState::FocusMode, &AZ::Render::EditorStateRequestsBus::Events::SetEnabled, true);
            AzToolsFramework::SetPrefabEditModeEffectEnabled(true);
            AzToolsFramework::EditorSettingsAPIBus::Broadcast(&AzToolsFramework::EditorSettingsAPIBus::Events::SaveSettingsRegistryFile);
            break;
        }
    default:
        AZ_Error("PrefabEditMode", false, AZStd::string::format("Unexpected edit mode: %zu", static_cast<size_t>(m_prefabEditMode)).c_str());
    }
}

void CViewportTitleDlg::UpdateDisplayInfo()
{
    if (m_viewportInformationMenu == nullptr)
    {
        // Nothing to update, just return;
        return;
    }

    AZ::AtomBridge::ViewportInfoDisplayState state = AZ::AtomBridge::ViewportInfoDisplayState::NoInfo;
    AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::BroadcastResult(
        state,
        &AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Events::GetDisplayState
    );

    m_noInformationAction->setChecked(false);
    m_normalInformationAction->setChecked(false);
    m_fullInformationAction->setChecked(false);
    m_compactInformationAction->setChecked(false);

    switch (state)
    {
        case AZ::AtomBridge::ViewportInfoDisplayState::NormalInfo:
            {
                m_normalInformationAction->setChecked(true);
                break;
            }
        case AZ::AtomBridge::ViewportInfoDisplayState::FullInfo:
            {
                m_fullInformationAction->setChecked(true);
                break;
            }
        case AZ::AtomBridge::ViewportInfoDisplayState::CompactInfo:
            {
                m_compactInformationAction->setChecked(true);
                break;
            }
        case AZ::AtomBridge::ViewportInfoDisplayState::NoInfo:
        default:
            {
                m_noInformationAction->setChecked(true);
                break;
            }
    }

    m_ui->m_debugInformationMenu->setChecked(state != AZ::AtomBridge::ViewportInfoDisplayState::NoInfo);
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnToggleDisplayInfo()
{
    AZ::AtomBridge::ViewportInfoDisplayState state = AZ::AtomBridge::ViewportInfoDisplayState::NoInfo;
    AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::BroadcastResult(
        state, &AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Events::GetDisplayState);
    state = aznumeric_cast<AZ::AtomBridge::ViewportInfoDisplayState>(
        (aznumeric_cast<int>(state) + 1) % aznumeric_cast<int>(AZ::AtomBridge::ViewportInfoDisplayState::Invalid));
    // SetDisplayState will fire OnViewportInfoDisplayStateChanged and notify us, no need to call UpdateDisplayInfo.
    AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Broadcast(
        &AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Events::SetDisplayState, state);
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::AddFOVMenus(QMenu* menu, std::function<void(float)> callback, const QStringList& customPresets)
{
    static const float fovs[] = {
        10.0f,
        20.0f,
        40.0f,
        55.0f,
        60.0f,
        70.0f,
        80.0f,
        90.0f
    };

    static const size_t fovCount = sizeof(fovs) / sizeof(fovs[0]);

    for (size_t i = 0; i < fovCount; ++i)
    {
        const float fov = fovs[i];
        QAction* action = menu->addAction(QString::number(fov));
        connect(action, &QAction::triggered, action, [fov, callback](){ callback(fov); });
    }

    menu->addSeparator();

    if (!customPresets.empty())
    {
        for (const QString& customPreset : customPresets)
        {
            if (customPreset.isEmpty())
            {
                break;
            }

            float fov = SandboxEditor::CameraDefaultFovRadians();
            bool ok;
            float f = customPreset.toFloat(&ok);
            if (ok)
            {
                fov = std::max(1.0f, f);
                fov = std::min(120.0f, f);
                QAction* action = menu->addAction(customPreset);
                connect(action, &QAction::triggered, action, [fov, callback](){ callback(fov); });
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnMenuFOVCustom()
{
    bool ok;
    int fovDegrees = QInputDialog::getInt(this, tr("Custom FOV"), QString(), 60, 1, 120, 1, &ok);

    if (ok)
    {
        m_pViewPane->SetViewportFOV(aznumeric_cast<float>(fovDegrees));
        // Update the custom presets.
        const QString text = QString::number(fovDegrees);
        UpdateCustomPresets(text, m_customFOVPresets);
        SaveCustomPresets("FOVPresets", "FOVPreset", m_customFOVPresets);
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::CreateFOVMenu()
{
    if (!m_fovMenu)
    {
        m_fovMenu = new QMenu("FOV", this);
    }

    m_fovMenu->clear();

    AddFOVMenus(
        m_fovMenu,
        [this](const float fovDegrees)
        {
            m_pViewPane->SetViewportFOV(fovDegrees);
        },
        m_customFOVPresets);

    if (!m_fovMenu->isEmpty())
    {
        m_fovMenu->addSeparator();
    }

    QAction* action = m_fovMenu->addAction(tr("Custom..."));
    connect(action, &QAction::triggered, this, &CViewportTitleDlg::OnMenuFOVCustom);
}

QMenu* const CViewportTitleDlg::GetFovMenu()
{
    CreateFOVMenu();
    return m_fovMenu;
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::AddAspectRatioMenus(QMenu* menu, std::function<void(int, int)> callback, const QStringList& customPresets)
{
    static const std::pair<unsigned int, unsigned int> ratios[] = {
        std::pair<unsigned int, unsigned int>(16, 9),
        std::pair<unsigned int, unsigned int>(16, 10),
        std::pair<unsigned int, unsigned int>(4, 3),
        std::pair<unsigned int, unsigned int>(5, 4)
    };

    static const size_t ratioCount = sizeof(ratios) / sizeof(ratios[0]);

    for (size_t i = 0; i < ratioCount; ++i)
    {
        int width = ratios[i].first;
        int height = ratios[i].second;
        QAction* action = menu->addAction(QString("%1:%2").arg(width).arg(height));
        connect(action, &QAction::triggered, action, [width, height, callback]() {callback(width, height); });
    }

    menu->addSeparator();

    for (const QString& customPreset : customPresets)
    {
        if (customPreset.isEmpty())
        {
            break;
        }

        static QRegularExpression regex(QStringLiteral("^(\\d+):(\\d+)$"));
        QRegularExpressionMatch matches = regex.match(customPreset);
        if (matches.hasMatch())
        {
            bool ok;
            unsigned int width = matches.captured(1).toInt(&ok);
            Q_ASSERT(ok);
            unsigned int height = matches.captured(2).toInt(&ok);
            Q_ASSERT(ok);
            QAction* action = menu->addAction(customPreset);
            connect(action, &QAction::triggered, action, [width, height, callback]() {callback(width, height); });
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnMenuAspectRatioCustom()
{
    const QRect viewportRect = m_pViewPane->GetViewport()->rect();
    const unsigned int width = viewportRect.width();
    const unsigned int height = viewportRect.height();

    int whGCD = gcd(width, height);
    CCustomAspectRatioDlg aspectRatioInputDialog(width / whGCD, height / whGCD, this);

    if (aspectRatioInputDialog.exec() == QDialog::Accepted)
    {
        const unsigned int aspectX = aspectRatioInputDialog.GetX();
        const unsigned int aspectY = aspectRatioInputDialog.GetY();

        m_pViewPane->SetAspectRatio(aspectX, aspectY);

        // Update the custom presets.
        UpdateCustomPresets(QString::fromLatin1("%1:%2").arg(aspectX).arg(aspectY), m_customAspectRatioPresets);
        SaveCustomPresets("AspectRatioPresets", "AspectRatioPreset", m_customAspectRatioPresets);
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::CreateAspectMenu()
{
    if (m_aspectMenu == nullptr)
    {
        m_aspectMenu = new QMenu("Aspect Ratio");
    }

    m_aspectMenu->clear();

    AddAspectRatioMenus(m_aspectMenu, [this](int width, int height) {m_pViewPane->SetAspectRatio(width, height); }, m_customAspectRatioPresets);
    if (!m_aspectMenu->isEmpty())
    {
        m_aspectMenu->addSeparator();
    }

    QAction* customAction = m_aspectMenu->addAction(tr("Custom..."));
    connect(customAction, &QAction::triggered, this, &CViewportTitleDlg::OnMenuAspectRatioCustom);
}

QMenu* const CViewportTitleDlg::GetAspectMenu()
{
    CreateAspectMenu();
    return m_aspectMenu;
}

QMenu* const CViewportTitleDlg::GetPrefabEditModeMenu()
{
    CreatePrefabEditModeMenu();
    return m_prefabEditModeMenu;
}

void CViewportTitleDlg::CreatePrefabEditModeMenu()
{
    if (m_prefabEditModeMenu == nullptr)
    {
        m_prefabEditModeMenu = new QMenu("Edit Mode");

        m_normalPrefabEditModeAction = new QAction(tr("Normal"), m_prefabEditModeMenu);
        m_normalPrefabEditModeAction->setCheckable(true);
        connect(m_normalPrefabEditModeAction, &QAction::triggered, this, &CViewportTitleDlg::SetNormalPrefabEditMode);
        m_prefabEditModeMenu->addAction(m_normalPrefabEditModeAction);

        m_monochromaticPrefabEditModeAction = new QAction(tr("Monochromatic"), m_prefabEditModeMenu);
        m_monochromaticPrefabEditModeAction->setCheckable(true);
        connect(m_monochromaticPrefabEditModeAction, &QAction::triggered, this, &CViewportTitleDlg::SetMonochromaticPrefabEditMode);
        m_prefabEditModeMenu->addAction(m_monochromaticPrefabEditModeAction);

        UpdatePrefabEditMode();
    }
}

QMenu* const CViewportTitleDlg::GetViewportInformationMenu()
{
    CreateViewportInformationMenu();
    return m_viewportInformationMenu;
}

void CViewportTitleDlg::CreateViewportInformationMenu()
{
    if (m_viewportInformationMenu == nullptr)
    {
        m_viewportInformationMenu = new QMenu("Viewport Information");

        m_noInformationAction = new QAction(tr("None"), m_viewportInformationMenu);
        m_noInformationAction->setCheckable(true);
        connect(m_noInformationAction, &QAction::triggered, this, &CViewportTitleDlg::SetNoViewportInfo);
        m_viewportInformationMenu->addAction(m_noInformationAction);

        m_normalInformationAction = new QAction(tr("Normal"), m_viewportInformationMenu);
        m_normalInformationAction->setCheckable(true);
        connect(m_normalInformationAction, &QAction::triggered, this, &CViewportTitleDlg::SetNormalViewportInfo);
        m_viewportInformationMenu->addAction(m_normalInformationAction);

        m_fullInformationAction = new QAction(tr("Full"), m_viewportInformationMenu);
        m_fullInformationAction->setCheckable(true);
        connect(m_fullInformationAction, &QAction::triggered, this, &CViewportTitleDlg::SetFullViewportInfo);
        m_viewportInformationMenu->addAction(m_fullInformationAction);

        m_compactInformationAction = new QAction(tr("Compact"), m_viewportInformationMenu);
        m_compactInformationAction->setCheckable(true);
        connect(m_compactInformationAction, &QAction::triggered, this, &CViewportTitleDlg::SetCompactViewportInfo);
        m_viewportInformationMenu->addAction(m_compactInformationAction);

        UpdateDisplayInfo();
    }
}

void CViewportTitleDlg::AddResolutionMenus(QMenu* menu, std::function<void(int, int)> callback, const QStringList& customPresets)
{
    struct SResolution
    {
        SResolution()
            : width(0)
            , height(0)
        {
        }

        SResolution(int w, int h)
            : width(w)
            , height(h)
        {
        }

        int width;
        int height;
    };

    static const SResolution resolutions[] = {
        SResolution(1280, 720),
        SResolution(1920, 1080),
        SResolution(2560, 1440),
        SResolution(2048, 858),
        SResolution(1998, 1080),
        SResolution(3840, 2160)
    };

    static const size_t resolutionCount = sizeof(resolutions) / sizeof(resolutions[0]);

    for (size_t i = 0; i < resolutionCount; ++i)
    {
        const int width = resolutions[i].width;
        const int height = resolutions[i].height;
        const QString text = QString::fromLatin1("%1 x %2").arg(width).arg(height);
        QAction* action = menu->addAction(text);
        connect(action, &QAction::triggered, action, [width, height, callback](){ callback(width, height); });
    }

    menu->addSeparator();

    for (const QString& customPreset : customPresets)
    {
        if (customPreset.isEmpty())
        {
            break;
        }

        static QRegularExpression regex(QStringLiteral("^(\\d+) x (\\d+)$"));
        QRegularExpressionMatch matches = regex.match(customPreset);
        if (matches.hasMatch())
        {
            bool ok;
            int width = matches.captured(1).toInt(&ok);
            Q_ASSERT(ok);
            int height = matches.captured(2).toInt(&ok);
            Q_ASSERT(ok);
            QAction* action = menu->addAction(customPreset);
            connect(action, &QAction::triggered, action, [width, height, callback](){ callback(width, height); });
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnMenuResolutionCustom()
{
    const QRect rectViewport = m_pViewPane->GetViewport()->rect();
    CCustomResolutionDlg resDlg(rectViewport.width(), rectViewport.height(), parentWidget());
    if (resDlg.exec() == QDialog::Accepted)
    {
        m_pViewPane->ResizeViewport(resDlg.GetWidth(), resDlg.GetHeight());
        // Update the custom presets.
        const QString text = QString::fromLatin1("%1 x %2").arg(resDlg.GetWidth()).arg(resDlg.GetHeight());
        UpdateCustomPresets(text, m_customResPresets);
        SaveCustomPresets("ResPresets", "ResPreset", m_customResPresets);
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::CreateResolutionMenu()
{
    if (!m_resolutionMenu)
    {
        m_resolutionMenu = new QMenu("Resolution");
    }

    m_resolutionMenu->clear();

    AddResolutionMenus(m_resolutionMenu, [this](int width, int height) { m_pViewPane->ResizeViewport(width, height); }, m_customResPresets);
    if (!m_resolutionMenu->isEmpty())
    {
        m_resolutionMenu->addSeparator();
    }

    QAction* action = m_resolutionMenu->addAction(tr("Custom..."));
    connect(action, &QAction::triggered, this, &CViewportTitleDlg::OnMenuResolutionCustom);
}

QMenu* const CViewportTitleDlg::GetResolutionMenu()
{
    CreateResolutionMenu();
    return m_resolutionMenu;
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnViewportSizeChanged(int width, int height)
{
    m_resolutionMenu->setTitle(QString::fromLatin1("Resolution:  %1 x %2").arg(width).arg(height));

    if (width != 0 && height != 0)
    {
        // Calculate greatest common divider of width & height
        int whGCD = gcd(width, height);

        m_aspectMenu->setTitle(QString::fromLatin1("Ratio:  %1:%2").arg(width / whGCD).arg(height / whGCD));
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnViewportFOVChanged(const float fovRadians)
{
    if (m_fovMenu)
    {
        const float fovDegrees = AZ::RadToDeg(fovRadians);
        m_fovMenu->setTitle(
            QString::fromLatin1("FOV:  %1%2").arg(qRound(fovDegrees)).arg(QString(QByteArray::fromPercentEncoding("%C2%B0"))));
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginGameMode:
    case eNotify_OnEndGameMode:
        UpdateMuteActionText();
        break;
    }
}

void CViewportTitleDlg::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    if (event == ESYSTEM_EVENT_RESIZE)
    {
        if (m_pViewPane)
        {
            const int eventWidth = aznumeric_cast<int>(wparam);
            const int eventHeight = aznumeric_cast<int>(lparam);
            const QWidget* viewport = m_pViewPane->GetViewport();

            // This should eventually be converted to an EBus to make it easy to connect to the correct viewport
            // sending the event.  But for now, just detect that we've gotten width/height values that match our
            // associated viewport
            if (viewport && (eventWidth == viewport->width()) && (eventHeight == viewport->height()))
            {
                OnViewportSizeChanged(eventWidth, eventHeight);
            }
        }
    }
}

void CViewportTitleDlg::LoadCustomPresets(const QString& section, const QString& keyName, QStringList& outCustompresets)
{
    QSettings settings("O3DE", "O3DE"); // Temporary solution until we have the global Settings class.
    settings.beginGroup(section);
    outCustompresets = settings.value(keyName).toStringList();
    settings.endGroup();
}

void CViewportTitleDlg::SaveCustomPresets(const QString& section, const QString& keyName, const QStringList& custompresets)
{
    QSettings settings("O3DE", "O3DE"); // Temporary solution until we have the global Settings class.
    settings.beginGroup(section);
    settings.setValue(keyName, custompresets);
    settings.endGroup();
}

void CViewportTitleDlg::UpdateCustomPresets(const QString& text, QStringList& custompresets)
{
    custompresets.removeAll(text);
    custompresets.push_front(text);
    if (custompresets.size() > MAX_NUM_CUSTOM_PRESETS)
    {
        custompresets.erase(custompresets.begin() + MAX_NUM_CUSTOM_PRESETS, custompresets.end()); // QList doesn't have resize()
    }
}

bool CViewportTitleDlg::eventFilter(QObject* object, QEvent* event)
{
    bool consumeEvent = false;

    // These events are forwarded from the toolbar that took ownership of our widgets
    if (event->type() == QEvent::MouseButtonDblClick)
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            OnMaximize();
            consumeEvent = true;
        }
    }
    else if (event->type() == QEvent::FocusIn)
    {
        parentWidget()->setFocus();
    }

    return QWidget::eventFilter(object, event) || consumeEvent;
}

void CViewportTitleDlg::OnBnClickedGotoPosition()
{
    emit ActionTriggered(ID_DISPLAY_GOTOPOSITION);
}

void CViewportTitleDlg::OnBnClickedMuteAudio()
{
    gSettings.bMuteAudio = !gSettings.bMuteAudio;
    if (gSettings.bMuteAudio)
    {
        LmbrCentral::AudioSystemComponentRequestBus::Broadcast(&LmbrCentral::AudioSystemComponentRequestBus::Events::GlobalMuteAudio);
    }
    else
    {
        LmbrCentral::AudioSystemComponentRequestBus::Broadcast(&LmbrCentral::AudioSystemComponentRequestBus::Events::GlobalUnmuteAudio);
    }

    UpdateMuteActionText();
}

void CViewportTitleDlg::UpdateMuteActionText()
{
    bool audioSystemConnected = false;
    LmbrCentral::AudioSystemComponentRequestBus::BroadcastResult(
        audioSystemConnected, &LmbrCentral::AudioSystemComponentRequestBus::Events::IsAudioSystemInitialized);
    if (audioSystemConnected)
    {
        m_audioMuteAction->setEnabled(true);
        m_audioMuteAction->setText(gSettings.bMuteAudio ? tr("Un-mute Audio") : tr("Mute Audio"));
    }
    else
    {
        m_audioMuteAction->setEnabled(false);
        m_audioMuteAction->setText(tr("Mute Audio: Enable Audio Gem"));
    }
}

inline double Round(double fVal, double fStep)
{
    if (fStep > 0.f)
    {
        fVal = int_round(fVal / fStep) * fStep;
    }
    return fVal;
}

void CViewportTitleDlg::CheckForCameraSpeedUpdate()
{
    const float currentCameraSpeedScale = SandboxEditor::CameraSpeedScale();
    if (currentCameraSpeedScale != m_prevMoveSpeedScale && !m_cameraSpinBox->hasFocus())
    {
        m_prevMoveSpeedScale = currentCameraSpeedScale;
        m_cameraSpinBox->setValue(currentCameraSpeedScale);
    }
}

void CViewportTitleDlg::OnGridSnappingToggled(const int state)
{
    m_gridSizeActionWidget->setEnabled(state == Qt::Checked);
    m_enableGridVisualizationCheckBox->setEnabled(state == Qt::Checked);
    SandboxEditor::SetGridSnapping(!SandboxEditor::GridSnappingEnabled());
}

void CViewportTitleDlg::OnAngleSnappingToggled(const int state)
{
    m_angleSizeActionWidget->setEnabled(state == Qt::Checked);
    SandboxEditor::SetAngleSnapping(!SandboxEditor::AngleSnappingEnabled());
}

void CViewportTitleDlg::OnGridSpinBoxChanged(const double value)
{
    SandboxEditor::SetGridSnappingSize(aznumeric_cast<float>(value));
}

void CViewportTitleDlg::OnAngleSpinBoxChanged(const double value)
{
    SandboxEditor::SetAngleSnappingSize(aznumeric_cast<float>(value));
}

void CViewportTitleDlg::UpdateOverFlowMenuState()
{
    const bool gridSnappingActive = SandboxEditor::GridSnappingEnabled();
    {
        QSignalBlocker signalBlocker(m_enableGridSnappingCheckBox);
        m_enableGridSnappingCheckBox->setChecked(gridSnappingActive);
    }
    m_gridSizeActionWidget->setEnabled(gridSnappingActive);

    const bool angleSnappingActive = SandboxEditor::AngleSnappingEnabled();
    {
        QSignalBlocker signalBlocker(m_enableAngleSnappingCheckBox);
        m_enableAngleSnappingCheckBox->setChecked(angleSnappingActive);
    }
    m_angleSizeActionWidget->setEnabled(angleSnappingActive);

    {
        QSignalBlocker signalBlocker(m_enableGridVisualizationCheckBox);
        m_enableGridVisualizationCheckBox->setChecked(SandboxEditor::ShowingGrid());
    }
}

 namespace
{
    void PyToggleHelpers()
    {
        AzToolsFramework::SetHelpersVisible(!AzToolsFramework::HelpersVisible());
    }

    bool PyIsHelpersShown()
    {
        return AzToolsFramework::HelpersVisible();
    }
} // namespace

namespace AzToolsFramework
{
    void ViewportTitleDlgPythonFuncsHandler::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // this will put these methods into the 'azlmbr.legacy.general' module
            auto addLegacyGeneral = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
            {
                methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Legacy/Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "legacy.general");
            };

            addLegacyGeneral(behaviorContext->Method("toggle_helpers", PyToggleHelpers, nullptr, "Toggles the display of helpers."));
            addLegacyGeneral(behaviorContext->Method("is_helpers_shown", PyIsHelpersShown, nullptr, "Gets the display state of helpers."));
        }
    }
} // namespace AzToolsFramework

#include "ViewportTitleDlg.moc"
#include <moc_ViewportTitleDlg.cpp>
