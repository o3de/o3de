/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

// CryCommon
#include <CryCommon/Maestro/Types/AnimNodeType.h>   // for AnimNodeType
#include <CryCommon/Maestro/Types/AnimValueType.h>  // for AnimValueType
#include <CryCommon/Maestro/Types/AnimParamType.h>  // for AnimParamType

// Editor
#include "Controls/ReflectedPropertyControl/ReflectedPropertyItem.h"    // for ReflectedPropertyItem
#include "TrackViewKeyPropertiesDlg.h"  // for CTrackViewKeyUIControls


class CCharacterKeyUIControls
    : public CTrackViewKeyUIControls
{
public:
    CSmartVariableArray mv_table;
    CSmartVariable<QString> mv_animation;
    CSmartVariable<bool> mv_loop;
    CSmartVariable<bool> mv_blendGap;
    CSmartVariable<bool> mv_inplace;
    CSmartVariable<float> mv_startTime;
    CSmartVariable<float> mv_endTime;
    CSmartVariable<float> mv_timeScale;

    void OnCreateVars() override
    {
        AddVariable(mv_table, "Key Properties");
        AddVariable(mv_table, mv_animation, "Animation", IVariable::DT_ANIMATION);
        AddVariable(mv_table, mv_loop, "Loop");
        AddVariable(mv_table, mv_blendGap, "Blend Gap");
        AddVariable(mv_table, mv_inplace, "In Place");
        AddVariable(mv_table, mv_startTime, "Start Time");
        AddVariable(mv_table, mv_endTime, "End Time");
        AddVariable(mv_table, mv_timeScale, "Time Scale");
        mv_timeScale->SetLimits(0.001f, 100.f);
    }
    bool SupportTrackType(const CAnimParamType& paramType, [[maybe_unused]] EAnimCurveType trackType, AnimValueType valueType) const override
    {
        return paramType == AnimParamType::Animation || valueType == AnimValueType::CharacterAnim;
    }
    bool OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys) override;
    void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys) override;

    unsigned int GetPriority() const override { return 1; }

    static const GUID& GetClassID()
    {
        // {EAA26453-6B74-4771-8FD1-14CDFF88E723}
        static const GUID guid =
        {
            0xeaa26453, 0x6b74, 0x4771, { 0x8f, 0xd1, 0x14, 0xcd, 0xff, 0x88, 0xe7, 0x23 }
        };
        return guid;
    }

protected:
    void ResetStartEndLimits(float characterKeyDuration);
};

void CCharacterKeyUIControls::ResetStartEndLimits(float characterKeyDuration)
{
    const float time_zero = .0f;
    float step = ReflectedPropertyItem::ComputeSliderStep(time_zero, characterKeyDuration);

    mv_startTime.GetVar()->SetLimits(time_zero, characterKeyDuration, step, true, true);
    mv_endTime.GetVar()->SetLimits(time_zero, characterKeyDuration, step, true, true);
}

bool CCharacterKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys)
{
    if (!selectedKeys.AreAllKeysOfSameType())
    {
        return false;
    }

    bool bAssigned = false;
    if (selectedKeys.GetKeyCount() == 1)
    {
        const CTrackViewKeyHandle& keyHandle = selectedKeys.GetKey(0);

        CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == AnimParamType::Animation || keyHandle.GetTrack()->GetValueType() == AnimValueType::CharacterAnim)
        {
            ICharacterKey charKey;
            keyHandle.GetKey(&charKey);

            // Find editor object who owns this node.
            const CTrackViewTrack* pTrack = keyHandle.GetTrack();
            if (pTrack->GetAnimNode()->GetType() == AnimNodeType::Component)
            {
                // no legacy entity was returned and the track's animNode is a component - try to get the AZ::EntityId from the component node's parent
                CTrackViewAnimNode* parentNode = static_cast<CTrackViewAnimNode*>(pTrack->GetAnimNode()->GetParentNode());
                if (parentNode)
                {
                    AZ::EntityId azEntityId = parentNode->GetAzEntityId();
                    if (azEntityId.IsValid())
                    {
                        static_assert(sizeof(AZ::EntityId) <= sizeof(AZ::u64), "Can't pack AZ::EntityId into a AZ::u64 in CCharacterKeyUIControls::OnKeySelectionChange.");
                        mv_animation->SetUserData(static_cast<AZ::u64>(azEntityId));
                    }
                }
            }

            mv_animation = charKey.m_animation.c_str();
            mv_loop = charKey.m_bLoop;
            mv_blendGap = charKey.m_bBlendGap;
            mv_inplace = charKey.m_bInPlace;
            mv_endTime = charKey.m_endTime;
            mv_startTime = charKey.m_startTime;
            mv_timeScale = charKey.m_speed;

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CCharacterKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!pSequence || !selectedKeys.AreAllKeysOfSameType())
    {
        return;
    }

    for (unsigned int keyIndex = 0, num = (int)selectedKeys.GetKeyCount(); keyIndex < num; keyIndex++)
    {
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);
        CTrackViewTrack* pTrack = keyHandle.GetTrack();
        const CAnimParamType paramType = pTrack->GetParameterType();

        if (paramType == AnimParamType::Animation || keyHandle.GetTrack()->GetValueType() == AnimValueType::CharacterAnim)
        {
            ICharacterKey charKey;
            keyHandle.GetKey(&charKey);

            if (mv_animation.GetVar() == pVar)
            {
                charKey.m_animation = ((QString)mv_animation).toUtf8().data();
                // This call is required to make sure that the newly set animation is properly triggered.
                pTrack->GetSequence()->Reset(false);
            }
            SyncValue(mv_loop, charKey.m_bLoop, false, pVar);
            SyncValue(mv_blendGap, charKey.m_bBlendGap, false, pVar);
            SyncValue(mv_inplace, charKey.m_bInPlace, false, pVar);
            SyncValue(mv_startTime, charKey.m_startTime, false, pVar);
            SyncValue(mv_endTime, charKey.m_endTime, false, pVar);
            SyncValue(mv_timeScale, charKey.m_speed, false, pVar);
            keyHandle.SetKey(&charKey);
        }
    }
}

REGISTER_QT_CLASS_DESC(CCharacterKeyUIControls, "TrackView.KeyUI.Character", "TrackViewKeyUI");
