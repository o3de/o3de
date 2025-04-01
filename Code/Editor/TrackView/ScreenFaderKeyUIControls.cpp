/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"


// CryCommon
#include <CryCommon/Maestro/Types/AnimParamType.h>

// Editor
#include "KeyUIControls.h"
#include "TrackViewKeyPropertiesDlg.h"


bool CScreenFaderKeyUIControls::OnKeySelectionChange(const CTrackViewKeyBundle& keys)
{
    if (!keys.AreAllKeysOfSameType())
    {
        return false;
    }

    bool bAssigned = false;
    if (keys.GetKeyCount() == 1)
    {
        const CTrackViewKeyHandle& keyHandle = keys.GetKey(0);

        CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == AnimParamType::ScreenFader)
        {
            IScreenFaderKey screenFaderKey;
            keyHandle.GetKey(&screenFaderKey);

            mv_fadeTime = screenFaderKey.m_fadeTime;
            mv_fadeColor = Vec3(screenFaderKey.m_fadeColor.GetR(), screenFaderKey.m_fadeColor.GetG(), screenFaderKey.m_fadeColor.GetB());
            mv_strTexture = screenFaderKey.m_strTexture.c_str();
            mv_bUseCurColor = screenFaderKey.m_bUseCurColor;
            mv_fadeType = (int)screenFaderKey.m_fadeType;
            mv_fadechangeType = (int)screenFaderKey.m_fadeChangeType;

            bAssigned = true;
        }
    }
    return bAssigned;
}

void CScreenFaderKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    if (!selectedKeys.AreAllKeysOfSameType())
    {
        return;
    }

    for (size_t keyIndex = 0, num = selectedKeys.GetKeyCount(); keyIndex < num; ++keyIndex)
    {
        CTrackViewKeyHandle selectedKey = selectedKeys.GetKey(static_cast<unsigned int>(keyIndex));

        CAnimParamType paramType = selectedKey.GetTrack()->GetParameterType();
        if (paramType == AnimParamType::ScreenFader)
        {
            IScreenFaderKey screenFaderKey;
            selectedKey.GetKey(&screenFaderKey);

            SyncValue(mv_fadeTime, screenFaderKey.m_fadeTime, false, pVar);

            SyncValue(mv_bUseCurColor, screenFaderKey.m_bUseCurColor, false, pVar);

            if (pVar == mv_fadeTime.GetVar())
            {
                screenFaderKey.m_fadeTime = MAX((float)mv_fadeTime, 0.f);
            }
            else if (pVar == mv_strTexture.GetVar())
            {
                QString sTexture = mv_strTexture;
                screenFaderKey.m_strTexture = sTexture.toUtf8().data();
            }
            else if (pVar == mv_fadeType.GetVar())
            {
                screenFaderKey.m_fadeType = IScreenFaderKey::EFadeType((int)mv_fadeType);
            }
            else if (pVar == mv_fadechangeType.GetVar())
            {
                screenFaderKey.m_fadeChangeType = IScreenFaderKey::EFadeChangeType((int)mv_fadechangeType);
            }
            else if (pVar == mv_fadeColor.GetVar())
            {
                Vec3 color = mv_fadeColor;
                screenFaderKey.m_fadeColor = AZ::Color(color.x, color.y, color.z, screenFaderKey.m_fadeType == IScreenFaderKey::eFT_FadeIn ? 1.f : 0.f);
            }

            selectedKey.SetKey(&screenFaderKey);
        }
    }
}
