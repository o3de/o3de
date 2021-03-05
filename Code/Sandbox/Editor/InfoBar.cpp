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
#include "EditTool.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_InfoBar.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

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

    m_enabledVector = false;
    m_bVectorLock = false;
    m_prevEditMode = 0;
    m_bSelectionLocked = false;
    m_bSelectionChanged = false;
    m_editTool = 0;
    m_bDragMode = false;
    m_prevMoveSpeed = 0;
    m_currValue = Vec3(-111, +222, -333); //this wasn't initialized. I don't know what a good value is
    m_oldMainVolume = 1.0f;

    GetIEditor()->RegisterNotifyListener(this);

    //audio request setup
    m_oMuteAudioRequest.pData       = &m_oMuteAudioRequestData;
    m_oUnmuteAudioRequest.pData = &m_oUnmuteAudioRequestData;

    OnInitDialog();

    connect(ui->m_vectorLock, &QToolButton::clicked, this, &CInfoBar::OnVectorLock);
    connect(ui->m_lockSelection, &QToolButton::clicked, this, &CInfoBar::OnLockSelection);

    auto comboBoxTextChanged = static_cast<void(QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged);
    connect(ui->m_moveSpeed, comboBoxTextChanged, this, &CInfoBar::OnUpdateMoveSpeedText);
    connect(ui->m_moveSpeed->lineEdit(), &QLineEdit::returnPressed, this, &CInfoBar::OnSpeedComboBoxEnter);

    connect(ui->m_posCtrl, &AzQtComponents::VectorInput::valueChanged, this, &CInfoBar::OnVectorChanged);

    // Hide some buttons from the expander menu
    AzQtComponents::Style::addClass(ui->m_posCtrl, "expanderMenu_hide");
    AzQtComponents::Style::addClass(ui->m_physDoStepBtn, "expanderMenu_hide");
    AzQtComponents::Style::addClass(ui->m_physSingleStepBtn, "expanderMenu_hide");

    // posCtrl is a VectorInput initialized via UI; as such, we can't construct it to have only 3 elements.
    // We can just hide the W element as it is unused.
    ui->m_posCtrl->getElements()[3]->setVisible(false);

    connect(ui->m_setVector, &QToolButton::clicked, this, &CInfoBar::OnBnClickedSetVector);
    connect(ui->m_physicsBtn, &QToolButton::clicked, this, &CInfoBar::OnBnClickedPhysics);
    connect(ui->m_physSingleStepBtn, &QToolButton::clicked, this, &CInfoBar::OnBnClickedSingleStepPhys);
    connect(ui->m_physDoStepBtn, &QToolButton::clicked, this, &CInfoBar::OnBnClickedDoStepPhys);
    connect(ui->m_syncPlayerBtn, &QToolButton::clicked, this, &CInfoBar::OnBnClickedSyncplayer);
    connect(ui->m_gotoPos, &QToolButton::clicked, this, &CInfoBar::OnBnClickedGotoPosition);
    connect(ui->m_muteBtn, &QToolButton::clicked, this, &CInfoBar::OnBnClickedMuteAudio);
    connect(ui->m_vrBtn, &QToolButton::clicked, this, &CInfoBar::OnBnClickedEnableVR);

    connect(this, &CInfoBar::ActionTriggered, MainWindow::instance()->GetActionManager(), &ActionManager::ActionTriggered);

    connect(ui->m_lockSelection, &QAbstractButton::toggled, ui->m_lockSelection, [this](bool checked) {
        ui->m_lockSelection->setToolTip(checked ? tr("Unlock Object Selection") : tr("Lock Object Selection"));
    });
    connect(ui->m_vectorLock, &QAbstractButton::toggled, ui->m_vectorLock, [this](bool checked) {
        ui->m_vectorLock->setToolTip(checked ? tr("Unlock Axis Vectors") : tr("Lock Axis Vectors"));
    });
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

    // hide old ui elements that are not valid with the new viewport interaction model
    if (GetIEditor()->IsNewViewportInteractionModelEnabled())
    {
        ui->m_lockSelection->setVisible(false);
        AzQtComponents::Style::addClass(ui->m_lockSelection, "expanderMenu_hide");
        ui->m_posCtrl->setVisible(false);
        AzQtComponents::Style::addClass(ui->m_posCtrl, "expanderMenu_hide");
        ui->m_setVector->setVisible(false);
        AzQtComponents::Style::addClass(ui->m_setVector, "expanderMenu_hide");
        ui->m_vectorLock->setVisible(false);
        AzQtComponents::Style::addClass(ui->m_vectorLock, "expanderMenu_hide");

        // As we're hiding some of the icons, we have an extra spacer to deal with.
        // We cannot set the visibility of separators, so we'll have to take it out.
        int separatorIndex = layout()->indexOf(ui->verticalSpacer_2);
        QLayoutItem* separator = layout()->takeAt(separatorIndex);

        // takeAt() removes the item from the layout; delete to avoid memory leaks.
        delete separator;
    }

    ui->m_moveSpeed->setValidator(new QDoubleValidator(m_minSpeed, m_maxSpeed, m_numDecimals, ui->m_moveSpeed));

    // Populate the presets in the ComboBox
    for (float presetValue : m_speedPresetValues)
    {
        ui->m_moveSpeed->addItem(QString().setNum(presetValue, 'f', m_numDecimals), presetValue);
    }

    SetSpeedComboBox(gSettings.cameraMoveSpeed);

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
    else if (event == eNotify_OnEditModeChange)
    {
        int emode = GetIEditor()->GetEditMode();
        switch (emode)
        {
        case eEditModeMove:
            ui->m_setVector->setToolTip(tr("Set Position of Selected Objects"));
            break;
        case eEditModeRotate:
            ui->m_setVector->setToolTip(tr("Set Rotation of Selected Objects"));
            break;
        case eEditModeScale:
            ui->m_setVector->setToolTip(tr("Set Scale of Selected Objects"));
            break;
        default:
            ui->m_setVector->setToolTip(tr("Set Position/Rotation/Scale of Selected Objects (None Selected)"));
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////

void CInfoBar::OnVectorChanged()
{
    SetVector(GetVector());
    OnVectorUpdate(false);
}

void CInfoBar::OnVectorUpdate(bool followTerrain)
{
    int emode = GetIEditor()->GetEditMode();
    if (emode != eEditModeMove && emode != eEditModeRotate && emode != eEditModeScale)
    {
        return;
    }

    Vec3 v = GetVector();

    ITransformManipulator* pManipulator = GetIEditor()->GetTransformManipulator();
    if (pManipulator)
    {
        CEditTool* pEditTool = GetIEditor()->GetEditTool();

        if (pEditTool)
        {
            Vec3 diff = v - m_lastValue;
            if (emode == eEditModeMove)
            {
                //GetIEditor()->RestoreUndo();
                pEditTool->OnManipulatorDrag(GetIEditor()->GetActiveView(), pManipulator, diff);
            }
            if (emode == eEditModeRotate)
            {
                diff = DEG2RAD(diff);
                //GetIEditor()->RestoreUndo();
                pEditTool->OnManipulatorDrag(GetIEditor()->GetActiveView(), pManipulator, diff);
            }
            if (emode == eEditModeScale)
            {
                //GetIEditor()->RestoreUndo();
                pEditTool->OnManipulatorDrag(GetIEditor()->GetActiveView(), pManipulator, diff);
            }
        }
        return;
    }

    CSelectionGroup* selection = GetIEditor()->GetObjectManager()->GetSelection();
    if (selection->IsEmpty())
    {
        return;
    }

    GetIEditor()->RestoreUndo();

    int referenceCoordSys = GetIEditor()->GetReferenceCoordSys();

    CBaseObject* obj = GetIEditor()->GetSelectedObject();

    Matrix34 tm;
    AffineParts ap;
    if (obj)
    {
        tm = obj->GetWorldTM();
        ap.SpectralDecompose(tm);
    }

    if (emode == eEditModeMove)
    {
        if (obj)
        {
            if (referenceCoordSys == COORDS_WORLD)
            {
                tm.SetTranslation(v);
                obj->SetWorldTM(tm);
            }
            else
            {
                obj->SetPos(v);
            }
        }
        else
        {
            GetIEditor()->GetSelection()->MoveTo(v, followTerrain ? CSelectionGroup::eMS_FollowTerrain : CSelectionGroup::eMS_None, referenceCoordSys);
        }
    }
    if (emode == eEditModeRotate)
    {
        if (obj)
        {
            AZ::Vector3 av = LYVec3ToAZVec3(v);
            AZ::Transform tr = AZ::ConvertEulerDegreesToTransform(av);
            Matrix34 lyTransform = AZTransformToLYTransform(tr);

            AffineParts newap;
            newap.SpectralDecompose(lyTransform);

            if (referenceCoordSys == COORDS_WORLD)
            {
                tm = Matrix34::Create(ap.scale, newap.rot, ap.pos);
                obj->SetWorldTM(tm);
            }
            else
            {
                obj->SetRotation(newap.rot);
            }
        }
        else
        {
            CBaseObject *refObj;
            CSelectionGroup* pGroup = GetIEditor()->GetSelection();
            if (pGroup && pGroup->GetCount() > 0)
            {
                refObj = pGroup->GetObject(0);
                AffineParts ap2;
                ap2.SpectralDecompose(refObj->GetWorldTM());
                Vec3 oldEulerRotation = AZVec3ToLYVec3(AZ::ConvertQuaternionToEulerDegrees(LYQuaternionToAZQuaternion(ap2.rot)));
                Vec3 diff = v - oldEulerRotation;
                GetIEditor()->GetSelection()->Rotate((Ang3)diff, referenceCoordSys);
            }
        }
    }
    if (emode == eEditModeScale)
    {
        if (v.x == 0 || v.y == 0 || v.z == 0)
        {
            return;
        }

        if (obj)
        {
            if (referenceCoordSys == COORDS_WORLD)
            {
                tm = Matrix34::Create(v, ap.rot, ap.pos);
                obj->SetWorldTM(tm);
            }
            else
            {
                obj->SetScale(v);
            }
        }
        else
        {
            GetIEditor()->GetSelection()->SetScale(v, referenceCoordSys);
        }
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

    /*
    // Get active viewport.
    int hx = marker.x / 2;
    int hy = marker.y / 2;
    if (m_heightMapX != hx || m_heightMapY != hy)
    {
        m_heightMapX = hx;
        m_heightMapY = hy;
        updateUI = true;
    }
    */

    RefCoordSys coordSys = GetIEditor()->GetReferenceCoordSys();
    bool bWorldSpace = GetIEditor()->GetReferenceCoordSys() == COORDS_WORLD;

    CSelectionGroup* selection = GetIEditor()->GetSelection();
    if (selection->GetCount() != m_numSelected)
    {
        m_numSelected = selection->GetCount();
        updateUI = true;
    }

    if (GetIEditor()->GetEditTool() != m_editTool)
    {
        updateUI = true;
        m_editTool = GetIEditor()->GetEditTool();
    }

    QString str;
    if (m_editTool)
    {
        str = m_editTool->GetStatusText();
        if (str != m_sLastText)
        {
            updateUI = true;
        }
    }

    if (updateUI)
    {
        if (!m_editTool)
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
        int settings = GetIEditor()->GetDisplaySettings()->GetSettings();
        bool noCollision = settings & SETTINGS_NOCOLLISION;

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

    bool bSelLocked = GetIEditor()->IsSelectionLocked();
    if (bSelLocked != m_bSelectionLocked)
    {
        m_bSelectionLocked = bSelLocked;
        ui->m_lockSelection->setChecked(m_bSelectionLocked);
    }


    if (GetIEditor()->GetSelection()->IsEmpty())
    {
        if (ui->m_lockSelection->isEnabled())
        {
            ui->m_lockSelection->setEnabled(false);
        }
    }
    else
    {
        if (!ui->m_lockSelection->isEnabled())
        {
            ui->m_lockSelection->setEnabled(true);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Update vector.
    //////////////////////////////////////////////////////////////////////////
    Vec3 v(0, 0, 0);
    bool enable = false;
    float min = 0, max = 10000;

    int emode = GetIEditor()->GetEditMode();
    ITransformManipulator* pManipulator = GetIEditor()->GetTransformManipulator();

    if (pManipulator)
    {
        AffineParts ap;
        ap.SpectralDecompose(pManipulator->GetTransformation(coordSys));

        if (emode == eEditModeMove)
        {
            v = ap.pos;
            enable = true;

            min = -64000;
            max = 64000;
        }
        if (emode == eEditModeRotate)
        {
            v = Vec3(RAD2DEG(Ang3::GetAnglesXYZ(Matrix33(ap.rot))));
            enable = true;
            min = -10000;
            max = 10000;
        }
        if (emode == eEditModeScale)
        {
            v = ap.scale;
            enable = true;
            min = -10000;
            max = 10000;
        }
    }
    else
    {
        if (selection->IsEmpty())
        {
            // Show marker position.
            EnableVector(false);
            SetVector(marker);
            SetVectorRange(-100000, 100000);
            return;
        }

        CBaseObject* obj = GetIEditor()->GetSelectedObject();
        if (!obj)
        {
            CSelectionGroup* pGroup = GetIEditor()->GetSelection();
            if (pGroup && pGroup->GetCount() > 0)
            {
                obj = pGroup->GetObject(0);
            }
        }

        if (obj)
        {
            v = obj->GetWorldPos();
        }

        if (emode == eEditModeMove)
        {
            if (obj)
            {
                if (bWorldSpace)
                {
                    v = obj->GetWorldTM().GetTranslation();
                }
                else
                {
                    v = obj->GetPos();
                }
            }
            enable = true;
            min = -64000;
            max = 64000;
        }
        if (emode == eEditModeRotate)
        {
            if (obj)
            {
                Quat objRot;
                if (bWorldSpace)
                {
                    AffineParts ap;
                    ap.SpectralDecompose(obj->GetWorldTM());
                    objRot = ap.rot;
                }
                else
                {
                    objRot = obj->GetRotation();
                }

                // Always convert objRot to v in order to ensure that the inspector and info bar are always in sync
                v = AZVec3ToLYVec3(AZ::ConvertQuaternionToEulerDegrees(LYQuaternionToAZQuaternion(objRot)));
            }
            enable = true;
            min = -10000;
            max = 10000;
        }
        if (emode == eEditModeScale)
        {
            if (obj)
            {
                if (bWorldSpace)
                {
                    AffineParts ap;
                    ap.SpectralDecompose(obj->GetWorldTM());
                    v = ap.scale;
                }
                else
                {
                    v = obj->GetScale();
                }
            }
            enable = true;
            min = -10000;
            max = 10000;
        }
    }

    bool updateDisplayVector = (m_currValue != v);

    // If Edit mode changed.
    if (m_prevEditMode != emode)
    {
        // Scale mode enables vector lock.
        SetVectorLock(emode == eEditModeScale);

        // Change undo strings.
        QString undoString("Modify Object(s)");
        int mode = GetIEditor()->GetEditMode();
        switch (mode)
        {
        case eEditModeMove:
            undoString = QStringLiteral("Move Object(s)");
            break;
        case eEditModeRotate:
            undoString = QStringLiteral("Rotate Object(s)");
            break;
        case eEditModeScale:
            undoString = QStringLiteral("Scale Object(s)");
            break;
        }

        // edit mode changed, we must update the number values
        updateDisplayVector = true;
    }

    SetVectorRange(min, max);
    EnableVector(enable);

    // if our selection changed, or if our display values are out of date
    if (m_bSelectionChanged)
    {
        updateDisplayVector = true;
        m_bSelectionChanged = false;
    }

    if (updateDisplayVector)
    {
        SetVector(v);
    }

    m_prevEditMode = emode;
}

inline double Round(double fVal, double fStep)
{
    if (fStep > 0.f)
    {
        fVal = int_round(fVal / fStep) * fStep;
    }
    return fVal;
}

void CInfoBar::SetVector(const Vec3& v)
{
    if (!m_bDragMode)
    {
        m_lastValue = m_currValue;
    }

    if (m_currValue != v)
    {
        ui->m_posCtrl->setValuebyIndex(v.x, 0);
        ui->m_posCtrl->setValuebyIndex(v.y, 1);
        ui->m_posCtrl->setValuebyIndex(v.z, 2);
        m_currValue = v;
    }
}

Vec3 CInfoBar::GetVector()
{
    Vec3 v;
    v.x = ui->m_posCtrl->getElements()[0]->getValue();
    v.y = ui->m_posCtrl->getElements()[1]->getValue();
    v.z = ui->m_posCtrl->getElements()[2]->getValue();
    m_currValue = v;
    return v;
}

void CInfoBar::EnableVector(bool enable)
{
    if (m_enabledVector != enable)
    {
        m_enabledVector = enable;
        ui->m_posCtrl->setEnabled(enable);
        ui->m_vectorLock->setEnabled(enable);
        ui->m_setVector->setEnabled(enable);
    }
}

void CInfoBar::SetVectorLock(bool bVectorLock)
{
    m_bVectorLock = bVectorLock;
    ui->m_vectorLock->setChecked(bVectorLock);
    GetIEditor()->SetAxisVectorLock(bVectorLock);
}

void CInfoBar::SetVectorRange(float min, float max)
{
    // Worth noting that this gets called every IdleUpdate, so it is necessary to make sure
    // setting the min/max doesn't result in the Qt event queue being pumped
    ui->m_posCtrl->setMinimum(min);
    ui->m_posCtrl->setMaximum(max);
}

void CInfoBar::OnVectorLock()
{
    SetVectorLock(!m_bVectorLock);
}

void CInfoBar::OnLockSelection()
{
    bool newLockSelectionValue = !m_bSelectionLocked;
    m_bSelectionLocked = newLockSelectionValue;
    ui->m_lockSelection->setChecked(newLockSelectionValue);
    GetIEditor()->LockSelection(newLockSelectionValue);
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

    ui->m_posCtrl->setEnabled(false);
    ui->m_posCtrl->getElements()[0]->setFixedWidth(width);
    ui->m_posCtrl->getElements()[1]->setFixedWidth(width);
    ui->m_posCtrl->getElements()[2]->setFixedWidth(width);
    ui->m_setVector->setEnabled(false);

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
void CInfoBar::OnBnClickedSetVector()
{
    emit ActionTriggered(ID_DISPLAY_SETVECTOR);
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
