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

#include "EditorDefs.h"

#include "InfoBar.h"

// Editor
#include "MainWindow.h"
#include "DisplaySettings.h"
#include "GameEngine.h"
#include "Include/ITransformManipulator.h"
#include "ActionManager.h"
#include "Settings.h"
#include "Objects/SelectionGroup.h"
#include "Include/IObjectManager.h"
#include "MathConversion.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_InfoBar.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

#include <QLineEdit>

#include <AzQtComponents/Components/Style.h>

#include "CryPhysicsDeprecation.h"

void BeautifyEulerAngles(Vec3& v)
{
    if (v.x + v.y + v.z >= 360.0f)
    {
        v.x = 180.0f - v.x;
        v.y = 180.0f - v.y;
        v.z = 180.0f - v.z;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CInfoBar dialog
CInfoBar::CInfoBar(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::CInfoBar)
{
    ui->setupUi(this);

    m_bSelectionChanged = false;
    m_bDragMode = false;
    m_prevMoveSpeed = 0;
    m_currValue = Vec3(-111, +222, -333); //this wasn't initialized. I don't know what a good value is
    m_oldMainVolume = 1.0f;

    GetIEditor()->RegisterNotifyListener(this);

    //audio request setup
    m_oMuteAudioRequest.pData       = &m_oMuteAudioRequestData;
    m_oUnmuteAudioRequest.pData = &m_oUnmuteAudioRequestData;

    OnInitDialog();

    auto comboBoxTextChanged = static_cast<void(QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged);
    connect(ui->m_moveSpeed, comboBoxTextChanged, this, &CInfoBar::OnUpdateMoveSpeedText);
    connect(ui->m_moveSpeed->lineEdit(), &QLineEdit::returnPressed, this, &CInfoBar::OnSpeedComboBoxEnter);

    // Hide some buttons from the expander menu
    AzQtComponents::Style::addClass(ui->m_physDoStepBtn, "expanderMenu_hide");
    AzQtComponents::Style::addClass(ui->m_physSingleStepBtn, "expanderMenu_hide");

    connect(ui->m_physicsBtn, &QToolButton::clicked, this, &CInfoBar::OnBnClickedPhysics);
    connect(ui->m_physSingleStepBtn, &QToolButton::clicked, this, &CInfoBar::OnBnClickedSingleStepPhys);
    connect(ui->m_physDoStepBtn, &QToolButton::clicked, this, &CInfoBar::OnBnClickedDoStepPhys);
    connect(ui->m_syncPlayerBtn, &QToolButton::clicked, this, &CInfoBar::OnBnClickedSyncplayer);
    connect(ui->m_gotoPos, &QToolButton::clicked, this, &CInfoBar::OnBnClickedGotoPosition);
    connect(ui->m_muteBtn, &QToolButton::clicked, this, &CInfoBar::OnBnClickedMuteAudio);
    connect(ui->m_vrBtn, &QToolButton::clicked, this, &CInfoBar::OnBnClickedEnableVR);

    connect(this, &CInfoBar::ActionTriggered, MainWindow::instance()->GetActionManager(), &ActionManager::ActionTriggered);

    connect(ui->m_physicsBtn, &QAbstractButton::toggled, ui->m_physicsBtn, [this](bool checked) {
        ui->m_physicsBtn->setToolTip(checked ? tr("Stop Simulation (Ctrl+P)") : tr("Simulate (Ctrl+P)"));
    });
    connect(ui->m_physSingleStepBtn, &QAbstractButton::toggled, ui->m_physSingleStepBtn, [this](bool checked) {
        ui->m_physSingleStepBtn->setToolTip(checked ? tr("Disable Physics/AI Single-step Mode ('<' in Game Mode)") : tr("Enable Physics/AI Single-step Mode ('<' in Game Mode)"));
    });
    connect(ui->m_syncPlayerBtn, &QAbstractButton::toggled, ui->m_syncPlayerBtn, [this](bool checked) {
        ui->m_syncPlayerBtn->setToolTip(checked ? tr("Synchronize Player with Camera") : tr("Move Player and Camera Separately"));
    });
    connect(ui->m_muteBtn, &QAbstractButton::toggled, ui->m_muteBtn, [this](bool checked) {
        ui->m_muteBtn->setToolTip(checked ? tr("Un-mute Audio") : tr("Mute Audio"));
    });
    connect(ui->m_vrBtn, &QAbstractButton::toggled, ui->m_vrBtn, [this](bool checked) {
        ui->m_vrBtn->setToolTip(checked ? tr("Disable VR Preview") : tr("Enable VR Preview"));
    });

    ui->m_moveSpeed->setValidator(new QDoubleValidator(m_minSpeed, m_maxSpeed, m_numDecimals, ui->m_moveSpeed));

    // Save off the move speed here since setting up the combo box can cause it to update values in the background.
    float cameraMoveSpeed = gSettings.cameraMoveSpeed;

    // Populate the presets in the ComboBox
    for (float presetValue : m_speedPresetValues)
    {
        ui->m_moveSpeed->addItem(QString().setNum(presetValue, 'f', m_numDecimals), presetValue);
    }

    SetSpeedComboBox(cameraMoveSpeed);

    ui->m_moveSpeed->setInsertPolicy(QComboBox::NoInsert);

    using namespace AzToolsFramework::ComponentModeFramework;
    EditorComponentModeNotificationBus::Handler::BusConnect(AzToolsFramework::GetEntityContextId());
}

//////////////////////////////////////////////////////////////////////////
CInfoBar::~CInfoBar()
{
    using namespace AzToolsFramework::ComponentModeFramework;
    EditorComponentModeNotificationBus::Handler::BusDisconnect();

    GetIEditor()->UnregisterNotifyListener(this);

    AZ::VR::VREventBus::Handler::BusDisconnect();
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    if (event == eNotify_OnIdleUpdate)
    {
        IdleUpdate();
    }
    else if (event == eNotify_OnBeginGameMode || event == eNotify_OnEndGameMode)
    {
        // Audio: determine muted state of audio
        //m_bMuted = gEnv->pAudioSystem->GetMainVolume() == 0.f;
        ui->m_muteBtn->setChecked(gSettings.bMuteAudio);
    }
    else if (event == eNotify_OnBeginLoad || event == eNotify_OnCloseScene)
    {
        // make sure AI/Physics is disabled on level load (CE-4229)
        if (GetIEditor()->GetGameEngine()->GetSimulationMode())
        {
            OnBnClickedPhysics();
        }

        ui->m_physicsBtn->setEnabled(false);
        ui->m_physSingleStepBtn->setEnabled(false);
        ui->m_physDoStepBtn->setEnabled(false);
    }
    else if (event == eNotify_OnEndLoad || event == eNotify_OnEndNewScene)
    {
        ui->m_physicsBtn->setEnabled(true);
        ui->m_physSingleStepBtn->setEnabled(true);
        ui->m_physDoStepBtn->setEnabled(true);
    }
    else if (event == eNotify_OnSelectionChange)
    {
        m_bSelectionChanged = true;
    }
}

void CInfoBar::IdleUpdate()
{
    if (!m_idleUpdateEnabled)
    {
        return;
    }

    bool updateUI = false;
    // Update Width/Height of selection rectangle.
    AABB box;
    GetIEditor()->GetSelectedRegion(box);
    float width = box.max.x - box.min.x;
    float height = box.max.y - box.min.y;
    if (m_width != width || m_height != height)
    {
        m_width = width;
        m_height = height;
        updateUI = true;
    }

    Vec3 marker = GetIEditor()->GetMarkerPosition();

    CSelectionGroup* selection = GetIEditor()->GetSelection();
    if (selection->GetCount() != m_numSelected)
    {
        m_numSelected = selection->GetCount();
        updateUI = true;
    }

    QString str;
    if (updateUI)
    {
        if (m_numSelected == 0)
        {
            str = tr("None Selected");
        }
        else if (m_numSelected == 1)
        {
            str = tr("1 Object Selected");
        }
        else
        {
            str = tr("%1 Objects Selected").arg(m_numSelected);
        }

        ui->m_statusText->setText(str);
        m_sLastText = str;
    }

    if (gSettings.cameraMoveSpeed != m_prevMoveSpeed &&
        !ui->m_moveSpeed->lineEdit()->hasFocus())
    {
        m_prevMoveSpeed = gSettings.cameraMoveSpeed;
        SetSpeedComboBox(gSettings.cameraMoveSpeed);
    }

    {
        bool bPhysics = GetIEditor()->GetGameEngine()->GetSimulationMode();
        if ((ui->m_physicsBtn->isChecked() && !bPhysics) ||
            (!ui->m_physicsBtn->isChecked() && bPhysics))
        {
            ui->m_physicsBtn->setChecked(bPhysics);
        }

        // Unsupported for Phyics:: atm
        bool bSingleStep = false;
        if (ui->m_physSingleStepBtn->isChecked() != bSingleStep)
        {
            ui->m_physSingleStepBtn->setChecked(bSingleStep);
        }

        bool bSyncPlayer = GetIEditor()->GetGameEngine()->IsSyncPlayerPosition();
        if ((!ui->m_syncPlayerBtn->isChecked() && !bSyncPlayer) ||
            (ui->m_syncPlayerBtn->isChecked() && bSyncPlayer))
        {
            ui->m_syncPlayerBtn->setChecked(!bSyncPlayer);
        }
    }

    // if our selection changed, or if our display values are out of date
    if (m_bSelectionChanged)
    {
        m_bSelectionChanged = false;
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

void CInfoBar::OnUpdateMoveSpeedText(const QString& text)
{
    gSettings.cameraMoveSpeed = aznumeric_cast<float>(Round(text.toDouble(), m_speedStep));
}

void CInfoBar::OnSpeedComboBoxEnter()
{
    ui->m_moveSpeed->clearFocus();
}

void CInfoBar::OnInitDialog()
{
    QFontMetrics metrics({});
    int width = metrics.boundingRect("-9999.99").width() * m_fieldWidthMultiplier;

    ui->m_moveSpeed->setFixedWidth(width);

    ui->m_physicsBtn->setEnabled(false);
    ui->m_physSingleStepBtn->setEnabled(false);
    ui->m_physDoStepBtn->setEnabled(false);
    
    ui->m_muteBtn->setChecked(gSettings.bMuteAudio);
    Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, gSettings.bMuteAudio ? m_oMuteAudioRequest : m_oUnmuteAudioRequest);

    //This is here just in case this class hasn't been created before
    //a VR headset was initialized
    ui->m_vrBtn->setEnabled(false);
    if (AZ::VR::HMDDeviceRequestBus::GetTotalNumOfEventHandlers() != 0)
    {
        ui->m_vrBtn->setEnabled(true);
    }

    AZ::VR::VREventBus::Handler::BusConnect();
}

void CInfoBar::OnHMDInitialized()
{
    ui->m_vrBtn->setEnabled(true);
}

void CInfoBar::OnHMDShutdown()
{
    ui->m_vrBtn->setEnabled(false);
}

void CInfoBar::OnBnClickedTerrainCollision()
{
    emit ActionTriggered(ID_TERRAIN_COLLISION);
}

void CInfoBar::OnBnClickedPhysics()
{
    if (!ui->m_physicsBtn->isEnabled())
    {
        return;
    }

    bool bPhysics = GetIEditor()->GetGameEngine()->GetSimulationMode();
    ui->m_physicsBtn->setChecked(bPhysics);
    emit ActionTriggered(ID_SWITCH_PHYSICS);

    if (bPhysics && ui->m_physSingleStepBtn->isChecked())
    {
        OnBnClickedSingleStepPhys();
    }
}

void CInfoBar::OnBnClickedSingleStepPhys()
{
    CRY_PHYSICS_REPLACEMENT_ASSERT();
}

void CInfoBar::OnBnClickedDoStepPhys()
{
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnBnClickedSyncplayer()
{
    emit ActionTriggered(ID_GAME_SYNCPLAYER);
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnBnClickedGotoPosition()
{
    emit ActionTriggered(ID_DISPLAY_GOTOPOSITION);
}

//////////////////////////////////////////////////////////////////////////
void CInfoBar::OnBnClickedMuteAudio()
{
    gSettings.bMuteAudio = !gSettings.bMuteAudio;

    Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, gSettings.bMuteAudio ? m_oMuteAudioRequest : m_oUnmuteAudioRequest);

    ui->m_muteBtn->setChecked(gSettings.bMuteAudio);
}

void CInfoBar::OnBnClickedEnableVR()
{
    gSettings.bEnableGameModeVR = !gSettings.bEnableGameModeVR;
    ui->m_vrBtn->setChecked(gSettings.bEnableGameModeVR);
}

void CInfoBar::EnteredComponentMode(const AZStd::vector<AZ::Uuid>& /*componentModeTypes*/)
{
    ui->m_physicsBtn->setDisabled(true);
}

void CInfoBar::LeftComponentMode(const AZStd::vector<AZ::Uuid>& /*componentModeTypes*/)
{
    ui->m_physicsBtn->setEnabled(true);
}

void CInfoBar::SetSpeedComboBox(double value)
{
    value = AZStd::clamp(Round(value, m_speedStep), m_minSpeed, m_maxSpeed);

    int index = ui->m_moveSpeed->findData(value);
    if (index != -1)
    {
        ui->m_moveSpeed->setCurrentIndex(index);
    }
    else
    {
        ui->m_moveSpeed->lineEdit()->setText(QString().setNum(value, 'f', m_numDecimals));
    }
}

#include <moc_InfoBar.cpp>
