/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "UiEditorAnimationBus.h"
#include "EditorDefs.h"
#include "UiAnimViewKeyPropertiesDlg.h"
#include "UiAnimViewDialog.h"
#include "UiAnimViewSequence.h"
#include "UiAnimViewTrack.h"
#include "UiAnimViewUndo.h"
#include "UiAVTrackEventKeyUIControls.h"

#include <ISplines.h>
#include <QVBoxLayout>
#include <Editor/Animation/ui_UiAnimViewTrackPropsDlg.h>


//////////////////////////////////////////////////////////////////////////
void CUiAnimViewKeyUIControls::OnInternalVariableChange(IVariable* pVar)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);

    CUiAnimViewSequenceNotificationContext context(pSequence);
    CUiAnimViewKeyBundle keys = pSequence->GetSelectedKeys();

    bool bAlreadyRecording = UiAnimUndo::IsRecording();
    if (!bAlreadyRecording)
    {
        // Try to start undo. This can't be done if restoring undo
        UiAnimUndoManager::Get()->Begin();

        if (UiAnimUndo::IsRecording())
        {
            CUiAnimViewSequence* pSequence2 = nullptr;
            UiEditorAnimationBus::BroadcastResult(pSequence2, &UiEditorAnimationBus::Events::GetCurrentSequence);
            pSequence2->StoreUndoForTracksWithSelectedKeys();
        }
        else
        {
            bAlreadyRecording = true;
        }
    }
    else
    {
        CUiAnimViewSequence* pSequence2 = nullptr;
        UiEditorAnimationBus::BroadcastResult(pSequence2, &UiEditorAnimationBus::Events::GetCurrentSequence);
        pSequence2->StoreUndoForTracksWithSelectedKeys();
    }

    OnUIChange(pVar, keys);

    if (!bAlreadyRecording)
    {
        UiAnimUndoManager::Get()->Accept("Change Keys");
    }
}

//////////////////////////////////////////////////////////////////////////

CUiAnimViewKeyPropertiesDlg::CUiAnimViewKeyPropertiesDlg(QWidget* hParentWnd)
    : QWidget(hParentWnd)
    , m_pLastTrackSelected(NULL)
{
    QVBoxLayout* l = new QVBoxLayout();
    l->setMargin(0);
    m_wndTrackProps = new CUiAnimViewTrackPropsDlg(this);
    l->addWidget(m_wndTrackProps);
    m_wndProps = new ReflectedPropertyControl(this);
    m_wndProps->setMinimumSize(50, 0);
    m_wndProps->Setup();
    m_wndProps->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    l->addWidget(m_wndProps);
    m_wndProps->SetStoreUndoByItems(false);

    setLayout(l);

    m_pVarBlock = new CVarBlock;

    // Add trackEvent Key Ui
    CUiAnimViewTrackEventKeyUIControls* trackEventControl = new CUiAnimViewTrackEventKeyUIControls;
    AZ_Assert(trackEventControl, "Failed to create Track Event Control.");
    m_keyControls.push_back(trackEventControl);

    CreateAllVars();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewKeyPropertiesDlg::OnVarChange([[maybe_unused]] IVariable* pVar)
{
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewKeyPropertiesDlg::CreateAllVars()
{
    for (int i = 0; i < (int)m_keyControls.size(); i++)
    {
        m_keyControls[i]->SetKeyPropertiesDlg(this);
        m_keyControls[i]->OnCreateVars();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewKeyPropertiesDlg::PopulateVariables()
{
    // Must first clear any selection in properties window.
    m_wndProps->ClearSelection();
    m_wndProps->RemoveAllItems();
    m_wndProps->AddVarBlock(m_pVarBlock);

    m_wndProps->SetUpdateCallback(AZStd::bind(&CUiAnimViewKeyPropertiesDlg::OnVarChange, this, AZStd::placeholders::_1));
    //m_wndProps->ExpandAll();


    ReloadValues();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewKeyPropertiesDlg::PopulateVariables(ReflectedPropertyControl& propCtrl)
{
    propCtrl.ClearSelection();
    propCtrl.RemoveAllItems();
    propCtrl.AddVarBlock(m_pVarBlock);

    propCtrl.ReloadValues();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewKeyPropertiesDlg::OnKeysChanged(CUiAnimViewSequence* pSequence)
{
    CUiAnimViewKeyBundle selectedKeys = pSequence->GetSelectedKeys();

    if (selectedKeys.GetKeyCount() > 0 && selectedKeys.AreAllKeysOfSameType())
    {
        CUiAnimViewTrack* pTrack = selectedKeys.GetKey(0).GetTrack();

        CUiAnimParamType paramType = pTrack->GetParameterType();
        EUiAnimCurveType trackType = pTrack->GetCurveType();
        EUiAnimValue valueType = pTrack->GetValueType();

        for (int i = 0; i < (int)m_keyControls.size(); i++)
        {
            if (m_keyControls[i]->SupportTrackType(paramType, trackType, valueType))
            {
                m_keyControls[i]->OnKeySelectionChange(selectedKeys);
                break;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewKeyPropertiesDlg::OnKeySelectionChanged(CUiAnimViewSequence* pSequence)
{
    if (nullptr == pSequence)
    {
        m_wndProps->ClearSelection();
        m_pVarBlock->DeleteAllVariables();
        m_wndProps->setEnabled(false);
        m_wndTrackProps->setEnabled(false);
        return;
    }

    CUiAnimViewKeyBundle selectedKeys = pSequence->GetSelectedKeys();

    m_wndTrackProps->OnKeySelectionChange(selectedKeys);

    bool bSelectChangedInSameTrack
        = m_pLastTrackSelected
            && selectedKeys.GetKeyCount() == 1
            && selectedKeys.GetKey(0).GetTrack() == m_pLastTrackSelected;

    if (selectedKeys.GetKeyCount() == 1)
    {
        m_pLastTrackSelected = selectedKeys.GetKey(0).GetTrack();
    }
    else
    {
        m_pLastTrackSelected = nullptr;
    }

    if (bSelectChangedInSameTrack)
    {
        m_wndProps->ClearSelection();
    }
    else
    {
        m_pVarBlock->DeleteAllVariables();
    }

    m_wndProps->setEnabled(false);
    bool bAssigned = false;
    if (selectedKeys.GetKeyCount() > 0 && selectedKeys.AreAllKeysOfSameType())
    {
        CUiAnimViewTrack* pTrack = selectedKeys.GetKey(0).GetTrack();

        CUiAnimParamType paramType = pTrack->GetParameterType();
        EUiAnimCurveType trackType = pTrack->GetCurveType();
        EUiAnimValue valueType = pTrack->GetValueType();

        for (int i = 0; i < (int)m_keyControls.size(); i++)
        {
            if (m_keyControls[i]->SupportTrackType(paramType, trackType, valueType))
            {
                if (!bSelectChangedInSameTrack)
                {
                    AddVars(m_keyControls[i]);
                }

                if (m_keyControls[i]->OnKeySelectionChange(selectedKeys))
                {
                    bAssigned = true;
                }

                break;
            }
        }
    }

    m_wndProps->setEnabled(bAssigned);

    if (bSelectChangedInSameTrack)
    {
        ReloadValues();
    }
    else
    {
        PopulateVariables();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewKeyPropertiesDlg::AddVars(CUiAnimViewKeyUIControls* pUI)
{
    CVarBlock* pVB = pUI->GetVarBlock();
    for (int i = 0, num = pVB->GetNumVariables(); i < num; i++)
    {
        IVariable* pVar = pVB->GetVariable(i);
        m_pVarBlock->AddVariable(pVar);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewKeyPropertiesDlg::ReloadValues()
{
    if (m_wndProps)
    {
        m_wndProps->ReloadValues();
    }
}

void CUiAnimViewKeyPropertiesDlg::OnSequenceChanged(CUiAnimViewSequence* sequence)
{
    OnKeySelectionChanged(sequence);
    m_wndTrackProps->OnSequenceChanged();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CUiAnimViewTrackPropsDlg::CUiAnimViewTrackPropsDlg(QWidget* parent /* = 0 */)
    : QWidget(parent)
    , ui(new Ui::CUiAnimViewTrackPropsDlg)
{
    ui->setupUi(this);
    connect(ui->TIME, SIGNAL(valueChanged(double)), this, SLOT(OnUpdateTime()));
}

CUiAnimViewTrackPropsDlg::~CUiAnimViewTrackPropsDlg()
{
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrackPropsDlg::OnSequenceChanged()
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);

    if (pSequence)
    {
        Range range = pSequence->GetTimeRange();
        ui->TIME->setRange(range.start, range.end);
    }
}


//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewTrackPropsDlg::OnKeySelectionChange(CUiAnimViewKeyBundle& selectedKeys)
{
    m_keyHandle = CUiAnimViewKeyHandle();

    if (selectedKeys.GetKeyCount() == 1)
    {
        m_keyHandle = selectedKeys.GetKey(0);
    }

    if (m_keyHandle.IsValid())
    {
        ui->TIME->setValue(m_keyHandle.GetTime());
        ui->PREVNEXT->setText(QString::number(m_keyHandle.GetIndex() + 1));

        ui->PREVNEXT->setEnabled(true);
        ui->TIME->setEnabled(true);
    }
    else
    {
        ui->PREVNEXT->setEnabled(FALSE);
        ui->TIME->setEnabled(FALSE);
    }
    return true;
}

void CUiAnimViewTrackPropsDlg::OnUpdateTime()
{
    if (!m_keyHandle.IsValid())
    {
        return;
    }

    UiAnimUndo undo("Change key time");
    UiAnimUndo::Record(new CUndoTrackObject(m_keyHandle.GetTrack()));

    const float time = (float) ui->TIME->value();
    m_keyHandle.SetTime(time);

    CUiAnimViewKeyHandle newKey = m_keyHandle.GetTrack()->GetKeyByTime(time);

    if (newKey != m_keyHandle)
    {
        SetCurrKey(newKey);
    }
}

void CUiAnimViewTrackPropsDlg::SetCurrKey(CUiAnimViewKeyHandle& keyHandle)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);

    if (keyHandle.IsValid())
    {
        UiAnimUndo undo("Select key");
        UiAnimUndo::Record(new CUndoAnimKeySelection(pSequence));

        m_keyHandle.Select(false);
        m_keyHandle = keyHandle;
        m_keyHandle.Select(true);
    }
}

#include <Animation/moc_UiAnimViewKeyPropertiesDlg.cpp>
