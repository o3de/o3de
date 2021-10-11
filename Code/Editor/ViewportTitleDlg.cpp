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
#include <QLabel>
#include <QInputDialog>

#include <AtomLyIntegration/AtomViewportDisplayInfo/AtomViewportInfoDisplayBus.h>

// Editor
#include "Settings.h"
#include "ViewPane.h"
#include "DisplaySettings.h"
#include "CustomResolutionDlg.h"
#include "CustomAspectRatioDlg.h"
#include "Include/IObjectManager.h"
#include "UsedResources.h"
#include "Objects/SelectionGroup.h"
#include "UsedResources.h"
#include "Include/IObjectManager.h"
#include "ActionManager.h"
#include "MainWindow.h"
#include "GameEngine.h"
#include "MathConversion.h"
#include "EditorViewportSettings.h"

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/algorithm.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

#include <LmbrCentral/Audio/AudioSystemComponentBus.h>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include "ui_ViewportTitleDlg.h"
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
#endif //!defined(Q_MOC_RUN)

// CViewportTitleDlg dialog

inline namespace Helpers
{
    void ToggleHelpers()
    {
        const bool newValue = !GetIEditor()->GetDisplaySettings()->IsDisplayHelpers();
        GetIEditor()->GetDisplaySettings()->DisplayHelpers(newValue);
        GetIEditor()->Notify(eNotify_OnDisplayRenderUpdate);

        if (newValue == false)
        {
            GetIEditor()->GetObjectManager()->SendEvent(EVENT_HIDE_HELPER);
        }
        AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Broadcast(
            &AzToolsFramework::ViewportInteraction::ViewportSettingNotifications::OnDrawHelpersChanged, newValue);
    }

    bool IsHelpersShown()
    {
        return GetIEditor()->GetDisplaySettings()->IsDisplayHelpers();
    }
}

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
            emit ViewportInfoStatusUpdated(static_cast<int>(state));
        }
    };
} //end anonymous namespace

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

    m_prevMoveSpeed = 0;

    m_pViewPane = nullptr;
    GetIEditor()->RegisterNotifyListener(this);
    GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);

    LoadCustomPresets("FOVPresets", "FOVPreset", m_customFOVPresets);
    LoadCustomPresets("AspectRatioPresets", "AspectRatioPreset", m_customAspectRatioPresets);
    LoadCustomPresets("ResPresets", "ResPreset", m_customResPresets);

    SetupCameraDropdownMenu();
    SetupResolutionDropdownMenu();
    SetupViewportInformationMenu();
    SetupHelpersButton();
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

    AZ::VR::VREventBus::Handler::BusConnect();

    OnInitDialog();
}

CViewportTitleDlg::~CViewportTitleDlg()
{
    AZ::VR::VREventBus::Handler::BusDisconnect();
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
    QMenu* cameraMenu = new QMenu(this);
    cameraMenu->addMenu(GetFovMenu());
    m_ui->m_cameraMenu->setMenu(cameraMenu);
    m_ui->m_cameraMenu->setPopupMode(QToolButton::InstantPopup);
    QObject::connect(cameraMenu, &QMenu::aboutToShow, this, &CViewportTitleDlg::CheckForCameraSpeedUpdate);

    QAction* gotoPositionAction = new QAction("Go to position", cameraMenu);
    connect(gotoPositionAction, &QAction::triggered, this, &CViewportTitleDlg::OnBnClickedGotoPosition);
    cameraMenu->addAction(gotoPositionAction);
    cameraMenu->addSeparator();

    auto cameraSpeedActionWidget = new QWidgetAction(cameraMenu);
    auto cameraSpeedContainer = new QWidget(cameraMenu);
    auto cameraSpeedLabel = new QLabel(tr("Camera Speed"), cameraMenu);
    m_cameraSpeed = new QComboBox(cameraMenu);
    m_cameraSpeed->setEditable(true);
    m_cameraSpeed->setValidator(new QDoubleValidator(m_minSpeed, m_maxSpeed, m_numDecimals, m_cameraSpeed));
    m_cameraSpeed->installEventFilter(this);

    QHBoxLayout* cameraSpeedLayout = new QHBoxLayout;
    cameraSpeedLayout->addWidget(cameraSpeedLabel);
    cameraSpeedLayout->addWidget(m_cameraSpeed);
    cameraSpeedContainer->setLayout(cameraSpeedLayout);
    cameraSpeedActionWidget->setDefaultWidget(cameraSpeedContainer);

    // Save off the move speed here since setting up the combo box can cause it to update values in the background.
    const float cameraMoveSpeed = SandboxEditor::CameraTranslateSpeed();
    // Populate the presets in the ComboBox
    for (float presetValue : m_speedPresetValues)
    {
        m_cameraSpeed->addItem(QString().setNum(presetValue, 'f', m_numDecimals), presetValue);
    }

    auto comboBoxTextChanged = static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged);

    SetSpeedComboBox(cameraMoveSpeed);
    m_cameraSpeed->setInsertPolicy(QComboBox::InsertAtBottom);
    m_cameraSpeed->setDuplicatesEnabled(false);
    connect(m_cameraSpeed, comboBoxTextChanged, this, &CViewportTitleDlg::OnUpdateMoveSpeedText);
    connect(m_cameraSpeed->lineEdit(), &QLineEdit::returnPressed, this, &CViewportTitleDlg::OnSpeedComboBoxEnter);

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

void CViewportTitleDlg::SetupViewportInformationMenu()
{
    // Setup the debug information button
    m_ui->m_debugInformationMenu->setMenu(GetViewportInformationMenu());
    connect(m_ui->m_debugInformationMenu, &QToolButton::clicked, this, &CViewportTitleDlg::OnToggleDisplayInfo);
    m_ui->m_debugInformationMenu->setPopupMode(QToolButton::MenuButtonPopup);

}

void CViewportTitleDlg::SetupHelpersButton()
{
    connect(m_ui->m_helpers, &QToolButton::clicked, this, &CViewportTitleDlg::OnToggleHelpers);
    m_ui->m_helpers->setChecked(Helpers::IsHelpersShown());
}

void CViewportTitleDlg::SetupOverflowMenu()
{
    // Setup the overflow menu
    QMenu* overFlowMenu = new QMenu(this);

    m_audioMuteAction = new QAction("Mute Audio", overFlowMenu);
    connect(m_audioMuteAction, &QAction::triggered, this, &CViewportTitleDlg::OnBnClickedMuteAudio);
    overFlowMenu->addAction(m_audioMuteAction);

    m_enableVRAction = new QAction("Enable VR Preview", overFlowMenu);
    connect(m_enableVRAction, &QAction::triggered, this, &CViewportTitleDlg::OnBnClickedEnableVR);
    overFlowMenu->addAction(m_enableVRAction);

    overFlowMenu->addSeparator();

    m_enableGridSnappingAction = new QAction("Enable Grid Snapping", overFlowMenu);
    connect(m_enableGridSnappingAction, &QAction::triggered, this, &CViewportTitleDlg::OnGridSnappingToggled);
    m_enableGridSnappingAction->setCheckable(true);
    overFlowMenu->addAction(m_enableGridSnappingAction);

    m_gridSizeActionWidget = new QWidgetAction(overFlowMenu);
    m_gridSpinBox = new AzQtComponents::DoubleSpinBox();
    m_gridSpinBox->setValue(SandboxEditor::GridSnappingSize());
    m_gridSpinBox->setMinimum(1e-2f);
    m_gridSpinBox->setToolTip(tr("Grid size"));

    QObject::connect(
        m_gridSpinBox, QOverload<double>::of(&AzQtComponents::DoubleSpinBox::valueChanged), this, &CViewportTitleDlg::OnGridSpinBoxChanged);

    m_gridSizeActionWidget->setDefaultWidget(m_gridSpinBox);
    overFlowMenu->addAction(m_gridSizeActionWidget);

    overFlowMenu->addSeparator();

    m_enableAngleSnappingAction = new QAction("Enable Angle Snapping", overFlowMenu);
    connect(m_enableAngleSnappingAction, &QAction::triggered, this, &CViewportTitleDlg::OnAngleSnappingToggled);
    m_enableAngleSnappingAction->setCheckable(true);
    overFlowMenu->addAction(m_enableAngleSnappingAction);

    m_angleSizeActionWidget = new QWidgetAction(overFlowMenu);
    m_angleSpinBox = new AzQtComponents::DoubleSpinBox();
    m_angleSpinBox->setValue(SandboxEditor::AngleSnappingSize());
    m_angleSpinBox->setMinimum(1e-2f);
    m_angleSpinBox->setToolTip(tr("Angle Snapping"));

    QObject::connect(
        m_angleSpinBox, QOverload<double>::of(&AzQtComponents::DoubleSpinBox::valueChanged), this,
        &CViewportTitleDlg::OnAngleSpinBoxChanged);

    m_angleSizeActionWidget->setDefaultWidget(m_angleSpinBox);
    overFlowMenu->addAction(m_angleSizeActionWidget);

    m_ui->m_overflowBtn->setMenu(overFlowMenu);
    m_ui->m_overflowBtn->setPopupMode(QToolButton::InstantPopup);
    connect(overFlowMenu, &QMenu::aboutToShow, this, &CViewportTitleDlg::UpdateOverFlowMenuState);

    UpdateMuteActionText();
}


//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::SetViewPane(CLayoutViewPane* pViewPane)
{
    if (m_pViewPane)
        m_pViewPane->disconnect(this);
    m_pViewPane = pViewPane;
    if (m_pViewPane)
        connect(this, &QWidget::customContextMenuRequested, m_pViewPane, &CLayoutViewPane::ShowTitleMenu);
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnInitDialog()
{
    // Add a child parented to us that listens for r_displayInfo changes.
    auto displayInfoHelper = new CViewportTitleDlgDisplayInfoHelper(this);
    connect(displayInfoHelper, &CViewportTitleDlgDisplayInfoHelper::ViewportInfoStatusUpdated, this, &CViewportTitleDlg::UpdateDisplayInfo);
    UpdateDisplayInfo();

    // This is here just in case this class hasn't been created before
    // a VR headset was initialized
    m_enableVRAction->setEnabled(false);
    if (AZ::VR::HMDDeviceRequestBus::GetTotalNumOfEventHandlers() != 0)
    {
        m_enableVRAction->setEnabled(true);
    }

    AZ::VR::VREventBus::Handler::BusConnect();

    QFontMetrics metrics({});
    int width = static_cast<int>(metrics.boundingRect("-9999.99").width() * m_fieldWidthMultiplier);

    m_cameraSpeed->setFixedWidth(width);

    bool isPrefabSystemEnabled = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(isPrefabSystemEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);

    if (isPrefabSystemEnabled)
    {
        m_prefabViewportFocusPathHandler = new AzToolsFramework::Prefab::PrefabViewportFocusPathHandler();
        m_prefabViewportFocusPathHandler->Initialize(m_ui->m_prefabFocusPath, m_ui->m_prefabFocusBackButton);
    }
    else
    {
        m_ui->m_prefabFocusPath->setEnabled(false);
        m_ui->m_prefabFocusBackButton->setEnabled(false);
        m_ui->m_prefabFocusPath->hide();
        m_ui->m_prefabFocusBackButton->hide();
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

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnToggleHelpers()
{
    Helpers::ToggleHelpers();
    m_ui->m_helpers->setChecked(Helpers::IsHelpersShown());
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

            float fov = gSettings.viewports.fDefaultFov;
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
    int fov = QInputDialog::getInt(this, tr("Custom FOV"), QString(), 60, 1, 120, 1, &ok);

    if (ok)
    {
        m_pViewPane->SetViewportFOV(static_cast<float>(fov));

        // Update the custom presets.
        const QString text = QString::number(fov);
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

    AddFOVMenus(m_fovMenu, [this](float f) { m_pViewPane->SetViewportFOV(f); }, m_customFOVPresets);
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
void CViewportTitleDlg::OnViewportFOVChanged(float fov)
{
    const float degFOV = RAD2DEG(fov);
    if (m_fovMenu)
    {
        m_fovMenu->setTitle(QString::fromLatin1("FOV:  %1%2").arg(qRound(degFOV)).arg(QString(QByteArray::fromPercentEncoding("%C2%B0"))));
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnDisplayRenderUpdate:
        m_ui->m_helpers->setChecked(Helpers::IsHelpersShown());
        break;
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
            const int eventWidth = static_cast<int>(wparam);
            const int eventHeight = static_cast<int>(lparam);
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
    if (object == m_cameraSpeed)
    {
        if (event->type() == QEvent::FocusIn)
        {
            QTimer::singleShot(
                0, this,
                [this]
                {
                    m_cameraSpeed->lineEdit()->selectAll();
                });
        }

        return m_cameraSpeed->eventFilter(object, event);
    }

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

void CViewportTitleDlg::OnHMDInitialized()
{
    m_enableVRAction->setEnabled(true);
}

void CViewportTitleDlg::OnHMDShutdown()
{
    m_enableVRAction->setEnabled(false);
}

void CViewportTitleDlg::OnBnClickedEnableVR()
{
    gSettings.bEnableGameModeVR = !gSettings.bEnableGameModeVR;

    m_enableVRAction->setText(gSettings.bEnableGameModeVR ? tr("Disable VR Preview") : tr("Enable VR Preview"));
}

inline double Round(double fVal, double fStep)
{
    if (fStep > 0.f)
    {
        fVal = int_round(fVal / fStep) * fStep;
    }
    return fVal;
}

void CViewportTitleDlg::SetSpeedComboBox(double value)
{
    value = AZStd::clamp(Round(value, m_speedStep), m_minSpeed, m_maxSpeed);

    int index = m_cameraSpeed->findData(value);
    if (index != -1)
    {
        m_cameraSpeed->setCurrentIndex(index);
    }
    else
    {
        m_cameraSpeed->lineEdit()->setText(QString().setNum(value, 'f', m_numDecimals));
    }
}

void CViewportTitleDlg::OnSpeedComboBoxEnter()
{
    m_cameraSpeed->clearFocus();
}

void CViewportTitleDlg::OnUpdateMoveSpeedText(const QString& text)
{
    SandboxEditor::SetCameraTranslateSpeed(aznumeric_cast<float>(Round(text.toDouble(), m_speedStep)));
}

void CViewportTitleDlg::CheckForCameraSpeedUpdate()
{
    const float currentCameraMoveSpeed = SandboxEditor::CameraTranslateSpeed();
    if (currentCameraMoveSpeed != m_prevMoveSpeed && !m_cameraSpeed->lineEdit()->hasFocus())
    {
        m_prevMoveSpeed = currentCameraMoveSpeed;
        SetSpeedComboBox(currentCameraMoveSpeed);
    }
}

void CViewportTitleDlg::OnGridSnappingToggled()
{
    m_gridSizeActionWidget->setEnabled(m_enableGridSnappingAction->isChecked());
    MainWindow::instance()->GetActionManager()->GetAction(AzToolsFramework::SnapToGrid)->trigger();
}

void CViewportTitleDlg::OnAngleSnappingToggled()
{
    m_angleSizeActionWidget->setEnabled(m_enableAngleSnappingAction->isChecked());
    MainWindow::instance()->GetActionManager()->GetAction(AzToolsFramework::SnapAngle)->trigger();
}

void CViewportTitleDlg::OnGridSpinBoxChanged(double value)
{
    SandboxEditor::SetGridSnappingSize(static_cast<float>(value));
}

void CViewportTitleDlg::OnAngleSpinBoxChanged(double value)
{
    SandboxEditor::SetAngleSnappingSize(static_cast<float>(value));
}

void CViewportTitleDlg::UpdateOverFlowMenuState()
{
    bool gridSnappingActive = MainWindow::instance()->GetActionManager()->GetAction(AzToolsFramework::SnapToGrid)->isChecked();
    {
        QSignalBlocker signalBlocker(m_enableGridSnappingAction);
        m_enableGridSnappingAction->setChecked(gridSnappingActive);
    }
    m_gridSizeActionWidget->setEnabled(gridSnappingActive);

    bool angleSnappingActive = MainWindow::instance()->GetActionManager()->GetAction(AzToolsFramework::SnapAngle)->isChecked();
    {
        QSignalBlocker signalBlocker(m_enableAngleSnappingAction);
        m_enableAngleSnappingAction->setChecked(angleSnappingActive);
    }
    m_angleSizeActionWidget->setEnabled(angleSnappingActive);
}

namespace
{
    void PyToggleHelpers()
    {
        GetIEditor()->GetDisplaySettings()->DisplayHelpers(!GetIEditor()->GetDisplaySettings()->IsDisplayHelpers());
        GetIEditor()->Notify(eNotify_OnDisplayRenderUpdate);

        if (GetIEditor()->GetDisplaySettings()->IsDisplayHelpers() == false)
        {
            GetIEditor()->GetObjectManager()->SendEvent(EVENT_HIDE_HELPER);
        }
    }

    bool PyIsHelpersShown()
    {
        return GetIEditor()->GetDisplaySettings()->IsDisplayHelpers();
    }
}

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
}

#include "ViewportTitleDlg.moc"
#include <moc_ViewportTitleDlg.cpp>
