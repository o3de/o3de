/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

// AzCore
#include <AzCore/std/algorithm.h>

// CryCommon
#include <CryCommon/Maestro/Types/AnimParamType.h>  // for AnimParamType

// Editor
#include "KeyUIControls.h"
#include "TrackViewKeyPropertiesDlg.h"              // for CTrackViewKeyUIControls// Editor

bool CTimeRangeKeyUIControls::OnKeySelectionChange(const CTrackViewKeyBundle& selectedKeys)
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
                timeRangeKey.m_endTime = AZStd::min(timeRangeKey.m_duration, timeRangeKey.m_endTime);
            }
            timeRangeKey.m_startTime = AZStd::min(timeRangeKey.m_duration, timeRangeKey.m_startTime);
            timeRangeKey.m_startTime = AZStd::min(timeRangeKey.m_endTime, timeRangeKey.m_startTime);

            keyHandle.SetKey(&timeRangeKey);
        }
    }
}
