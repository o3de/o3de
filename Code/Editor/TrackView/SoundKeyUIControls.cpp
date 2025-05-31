/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

// CryCommon
#include <CryCommon/Maestro/Types/AnimParamType.h>  // for AnimParamType

// Editor
#include "KeyUIControls.h"
#include "TrackViewKeyPropertiesDlg.h"  // for CTrackViewKeyUIControls

bool CSoundKeyUIControls::OnKeySelectionChange(const CTrackViewKeyBundle& selectedKeys)
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
        if (paramType == AnimParamType::Sound)
        {
            ISoundKey soundKey;
            keyHandle.GetKey(&soundKey);

            mv_startTrigger = soundKey.sStartTrigger.c_str();
            mv_stopTrigger = soundKey.sStopTrigger.c_str();
            mv_duration = soundKey.fDuration;
            mv_customColor = AZ::Vector3(soundKey.customColor.x, soundKey.customColor.y, soundKey.customColor.z);

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CSoundKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!pSequence || !selectedKeys.AreAllKeysOfSameType())
    {
        return;
    }

    for (unsigned int keyIndex = 0; keyIndex < selectedKeys.GetKeyCount(); ++keyIndex)
    {
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);

        CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == AnimParamType::Sound)
        {
            ISoundKey soundKey;
            keyHandle.GetKey(&soundKey);

            if (pVar == mv_startTrigger.GetVar())
            {
                QString sFilename = mv_startTrigger;
                soundKey.sStartTrigger = sFilename.toUtf8().data();
            }
            else if (pVar == mv_stopTrigger.GetVar())
            {
                QString sFilename = mv_stopTrigger;
                soundKey.sStopTrigger = sFilename.toUtf8().data();
            }
            else if (pVar == mv_customColor.GetVar())
            {
                AZ::Vector3 vector3 = mv_customColor;
                soundKey.customColor = Vec3(vector3.GetX(), vector3.GetY(), vector3.GetZ());
            }

            SyncValue(mv_duration, soundKey.fDuration, false, pVar);

            keyHandle.SetKey(&soundKey);
        }
    }
}
