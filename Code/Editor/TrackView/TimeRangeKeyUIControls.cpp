/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

// CryCommon
#include <CryCommon/Maestro/Types/AnimParamType.h>  // for AnimParamType

// Editor
#include "TrackViewKeyPropertiesDlg.h"              // for CTrackViewKeyUIControls// Editor


//////////////////////////////////////////////////////////////////////////
class CTimeRangeKeyUIControls
    : public CTrackViewKeyUIControls
{
public:
    CSmartVariableArray mv_table;
    CSmartVariable<float> mv_startTime;
    CSmartVariable<float> mv_endTime;
    CSmartVariable<float> mv_timeScale;
    CSmartVariable<bool> mv_bLoop;

    virtual void OnCreateVars()
    {
        AddVariable(mv_table, "Key Properties");
        AddVariable(mv_table, mv_startTime, "Start Time");
        AddVariable(mv_table, mv_endTime, "End Time");
        AddVariable(mv_table, mv_timeScale, "Time Scale");
        AddVariable(mv_table, mv_bLoop, "Loop");
        mv_timeScale->SetLimits(0.001f, 100.f);
    }
    bool SupportTrackType(const CAnimParamType& paramType, [[maybe_unused]] EAnimCurveType trackType, [[maybe_unused]] AnimValueType valueType) const
    {
        return paramType == AnimParamType::TimeRanges;
    }
    virtual bool OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys);
    virtual void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys);

    virtual unsigned int GetPriority() const { return 1; }

    static const GUID& GetClassID()
    {
        // {E977A6F4-CEC1-4c67-8735-28721B3F6FEF}
        static const GUID guid = {
            0xe977a6f4, 0xcec1, 0x4c67, { 0x87, 0x35, 0x28, 0x72, 0x1b, 0x3f, 0x6f, 0xef }
        };
        return guid;
    }
};

//////////////////////////////////////////////////////////////////////////
bool CTimeRangeKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys)
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
        if (paramType == AnimParamType::TimeRanges)
        {
            ICharacterKey timeRangeKey;
            keyHandle.GetKey(&timeRangeKey);

            mv_endTime = timeRangeKey.m_endTime;
            mv_startTime = timeRangeKey.m_startTime;
            mv_timeScale = timeRangeKey.m_speed;
            mv_bLoop = timeRangeKey.m_bLoop;

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CTimeRangeKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!pSequence || !selectedKeys.AreAllKeysOfSameType())
    {
        return;
    }

    for (unsigned int keyIndex = 0, num = (int)selectedKeys.GetKeyCount(); keyIndex < num; keyIndex++)
    {
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);

        CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == AnimParamType::TimeRanges)
        {
            ITimeRangeKey timeRangeKey;
            keyHandle.GetKey(&timeRangeKey);

            SyncValue(mv_startTime, timeRangeKey.m_startTime, false, pVar);
            SyncValue(mv_endTime, timeRangeKey.m_endTime, false, pVar);
            SyncValue(mv_timeScale, timeRangeKey.m_speed, false, pVar);
            SyncValue(mv_bLoop, timeRangeKey.m_bLoop, false, pVar);

            // Clamp values
            if (!timeRangeKey.m_bLoop)
            {
                timeRangeKey.m_endTime = std::min(timeRangeKey.m_duration, timeRangeKey.m_endTime);
            }
            timeRangeKey.m_startTime = std::min(timeRangeKey.m_duration, timeRangeKey.m_startTime);
            timeRangeKey.m_startTime = std::min(timeRangeKey.m_endTime, timeRangeKey.m_startTime);

            keyHandle.SetKey(&timeRangeKey);
        }
    }
}

REGISTER_QT_CLASS_DESC(CTimeRangeKeyUIControls, "TrackView.KeyUI.TimeRange", "TrackViewKeyUI");
