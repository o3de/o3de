/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include <AzCore/StringFunc/StringFunc.h>

// CryCommon
#include <CryCommon/Maestro/Types/AnimParamType.h>

// Editor
#include "Controls/ReflectedPropertyControl/ReflectedPropertyItem.h"
#include "KeyUIControls.h"
#include "TrackViewKeyPropertiesDlg.h"

bool CScreenFaderKeyUIControls::OnKeySelectionChange(const CTrackViewKeyBundle& keys)
{
    if (!keys.AreAllKeysOfSameType() || keys.GetKeyCount() < 1)
    {
        AZ_Assert(false, "Expected valid single selected key or valid keys of same type.");
        return false;
    }

    const CTrackViewKeyHandle& keyHandle = keys.GetKey(0);
    auto pTrack = keyHandle.GetTrack();
    if (!keyHandle.IsValid() || !pTrack || !pTrack->GetAnimTrack())
    {
        AZ_Assert(keyHandle.IsValid(), "Expected valid key handle.");
        AZ_Assert(pTrack, "Expected valid track in key handle.");
        return false;
    }

    CAnimParamType paramType = pTrack->GetParameterType();
    if (paramType != AnimParamType::ScreenFader)
    {
        AZ_Assert(false, "Expected to be called for ScreenFader key selection.");
        return false;
    }

    auto pAnimTrack = pTrack->GetAnimTrack();
    if (!pAnimTrack)
    {
        AZ_Assert(false, "Expected valid animation track in key handle.");
        return false;
    }

    IScreenFaderKey screenFaderKey;
    keyHandle.GetKey(&screenFaderKey);

    m_skipOnUIChange = true;

    // Recalculate a valid m_fadeTime range
    const auto minFadeTime = pAnimTrack->GetMinKeyTimeDelta();
    auto maxFadeTime = AZStd::max(minFadeTime, pAnimTrack->GetTimeRange().end - screenFaderKey.time);
    const auto nextKeyHandle = const_cast<CTrackViewTrack*>(pTrack)->GetNextKey(screenFaderKey.time);
    if (nextKeyHandle.IsValid())
    {
        maxFadeTime = AZStd::max(minFadeTime, nextKeyHandle.GetTime() - screenFaderKey.time);
    }
    if (screenFaderKey.m_fadeTime < minFadeTime || screenFaderKey.m_fadeTime > maxFadeTime)
    {
        screenFaderKey.m_fadeTime = AZStd::clamp(screenFaderKey.m_fadeTime, minFadeTime, maxFadeTime);
    }
    mv_fadeTime = screenFaderKey.m_fadeTime;
    float step = ReflectedPropertyItem::ComputeSliderStep(minFadeTime, maxFadeTime);
    mv_fadeTime.GetVar()->SetLimits(minFadeTime, maxFadeTime, step, true, true);

    // Could be single item with X,Y,Z fields, reflected AZ::Vector3 <-> CReflectedVarColor3, - as commented out below ...
    //mv_fadeColor = screenFaderKey.m_fadeColor.GetAsVector3() * colorMax;
    // ... but properly named full-featured group of three R,G,B items seems more user-friendly.
    step = ReflectedPropertyItem::ComputeSliderStep(colorMin, colorMax);
    mv_fadeColorR = screenFaderKey.m_fadeColor.GetR() * colorMax;
    mv_fadeColorR.GetVar()->SetLimits(colorMin, colorMax, step, true, true);
    mv_fadeColorG = screenFaderKey.m_fadeColor.GetG() * colorMax;
    mv_fadeColorG.GetVar()->SetLimits(colorMin, colorMax, step, true, true);
    mv_fadeColorB = screenFaderKey.m_fadeColor.GetB() * colorMax;
    mv_fadeColorB.GetVar()->SetLimits(colorMin, colorMax, step, true, true);

    mv_strTexture = screenFaderKey.m_strTexture.c_str();

    mv_bUseCurColor = screenFaderKey.m_bUseCurColor;

    mv_fadeType = (int)screenFaderKey.m_fadeType;
    mv_fadechangeType = (int)screenFaderKey.m_fadeChangeType;

    m_skipOnUIChange = false;

    return true;
}

void CScreenFaderKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    if (!selectedKeys.AreAllKeysOfSameType() || m_skipOnUIChange)
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
                // Normalize texture path, skipping occasional backed image extensions
                AZStd::string assetPath = mv_strTexture;
                AZStd::string assetFolderPath;
                AZStd::string assetName;
                AZStd::string extension;
                AzFramework::StringFunc::Path::Split(assetPath.c_str(), nullptr, &assetFolderPath, &assetName, &extension);
                if (extension == ".streamingimage" || extension == ".imagemipchain")
                {
                    assetPath = assetFolderPath + "/" + assetName;
                }
                else
                {
                    assetPath = assetFolderPath + "/" + assetName + extension;
                }

                screenFaderKey.m_strTexture = assetPath;
            }
            else if (pVar == mv_fadeType.GetVar())
            {
                screenFaderKey.m_fadeType = IScreenFaderKey::EFadeType((int)mv_fadeType);
            }
            else if (pVar == mv_fadechangeType.GetVar())
            {
                screenFaderKey.m_fadeChangeType = IScreenFaderKey::EFadeChangeType((int)mv_fadechangeType);
            }
            else if (pVar == mv_fadeColorR.GetVar() || pVar == mv_fadeColorG.GetVar() || pVar == mv_fadeColorB.GetVar())
            {
                // For single CSmartVariable<AZ::Vector3> mv_fadeColor; - with XYZ sub-items, code would be as commented out below:
                //AZ::Vector3 color = mv_fadeColor;
                //screenFaderKey.m_fadeColor = AZ::Color(color.GetX(), color.GetY(), color.GetZ(), screenFaderKey.m_fadeType == IScreenFaderKey::eFT_FadeIn ? 1.f : 0.f);

                // Gather 3 float items reducing to (0.0 .. 1.0) range
                screenFaderKey.m_fadeColor = AZ::Color(
                    mv_fadeColorR / colorMax,
                    mv_fadeColorG / colorMax,
                    mv_fadeColorB / colorMax,
                    screenFaderKey.m_fadeType == IScreenFaderKey::eFT_FadeIn ? 1.f : 0.f);
            }

            selectedKey.SetKey(&screenFaderKey);
        }
    }
}
