/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"


// AzCore
#include <AzCore/Asset/AssetManagerBus.h>

// CryCommon
#include <CryCommon/Maestro/Bus/SequenceComponentBus.h>
#include <CryCommon/Maestro/Types/AnimNodeType.h>
#include <CryCommon/Maestro/Types/AnimValueType.h>
#include <CryCommon/Maestro/Types/AssetBlendKey.h>

// Editor
#include "KeyUIControls.h"
#include "TrackViewKeyPropertiesDlg.h"
#include "Controls/ReflectedPropertyControl/ReflectedPropertyItem.h"

void CAssetBlendKeyUIControls::ResetStartEndLimits(float assetBlendKeyDuration)
{
    float step = ReflectedPropertyItem::ComputeSliderStep(0.0f, assetBlendKeyDuration);
    mv_blendInTime.GetVar()->SetLimits(0.0f, assetBlendKeyDuration, step, true, true);
    mv_blendOutTime.GetVar()->SetLimits(0.0f, assetBlendKeyDuration, step, true, true);
}

bool CAssetBlendKeyUIControls::OnKeySelectionChange(const CTrackViewKeyBundle& selectedKeys)
{
    if (selectedKeys.GetKeyCount() < 1  || !selectedKeys.AreAllKeysOfSameType())
    {
        return false;
    }

    const CTrackViewKeyHandle& keyHandle = selectedKeys.GetKey(0); // Get the 1st key for editing
    if (keyHandle.GetTrack()->GetValueType() != AnimValueType::AssetBlend)
    {
        return false;
    }

    AZ::IAssetBlendKey assetBlendKey;
    keyHandle.GetKey(&assetBlendKey);

    // Find editor object who owns this node.
    const CTrackViewTrack* pTrack = keyHandle.GetTrack();
    if (pTrack && pTrack->GetAnimNode()->GetType() == AnimNodeType::Component)
    {
        m_componentId = pTrack->GetAnimNode()->GetComponentId();

        // try to get the AZ::EntityId from the component node's parent
        CTrackViewAnimNode* parentNode = static_cast<CTrackViewAnimNode*>(pTrack->GetAnimNode()->GetParentNode());
        if (parentNode)
        {
            m_entityId = parentNode->GetAzEntityId();
        }
    }

    m_skipOnUIChange = true;

    if (assetBlendKey.m_assetId.IsValid() && assetBlendKey.m_duration > 0.0f)
    {
        mv_asset->SetUserData(assetBlendKey.m_assetId.m_subId);
        mv_asset->SetDisplayValue(assetBlendKey.m_assetId.m_guid.ToString<AZStd::string>().c_str());

        mv_loop = assetBlendKey.m_bLoop;

        {
            float step = ReflectedPropertyItem::ComputeSliderStep(AZ::IAssetBlendKey::s_minSpeed, AZ::IAssetBlendKey::s_maxSpeed);
            mv_timeScale = AZStd::clamp(assetBlendKey.m_speed, AZ::IAssetBlendKey::s_minSpeed, AZ::IAssetBlendKey::s_maxSpeed);
            mv_timeScale->SetLimits(AZ::IAssetBlendKey::s_minSpeed, AZ::IAssetBlendKey::s_maxSpeed, step, true, true);
        }

        mv_blendInTime = assetBlendKey.m_blendInTime;
        mv_blendOutTime = assetBlendKey.m_blendOutTime;
        ResetStartEndLimits(assetBlendKey.m_duration);
    }
    else
    {
        mv_asset = "";
        mv_loop = false;
        mv_timeScale = 1.0f;
        mv_blendInTime = 0.0f;
        mv_blendOutTime = 0.0f;
    }

    m_skipOnUIChange = false;

    return true;
}

void CAssetBlendKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!pSequence || !selectedKeys.AreAllKeysOfSameType() || m_skipOnUIChange || selectedKeys.GetKeyCount() < 1)
    {
        return;
    }

    CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(0);  // Get the 1st key for editing
    CTrackViewTrack* pTrack = keyHandle.GetTrack();
    if (keyHandle.GetTrack()->GetValueType() != AnimValueType::AssetBlend)
    {
        return;
    }

    AZ::IAssetBlendKey assetBlendKey;
    keyHandle.GetKey(&assetBlendKey);

    if (mv_asset.GetVar() == pVar)
    {
        AZStd::string stringGuid = mv_asset->GetDisplayValue().toLatin1().data();
        if (!stringGuid.empty())
        {
            AZ::Uuid guid(stringGuid.c_str(), stringGuid.length());
            AZ::u32 subId = mv_asset->GetUserData().value<AZ::u32>();
            assetBlendKey.m_assetId = AZ::Data::AssetId(guid, subId);

            // Lookup Filename by assetId and get the filename part of the description
            AZStd::string assetPath;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, assetBlendKey.m_assetId);

            assetBlendKey.m_description = "";
            if (!assetPath.empty())
            {
                AZStd::string filename;
                if (AzFramework::StringFunc::Path::GetFileName(assetPath.c_str(), filename))
                {
                    assetBlendKey.m_description = filename;
                }
            }
        }

        // This call is required to make sure that the newly set animation is properly triggered.
        pTrack->GetSequence()->Reset(false);
    }

    if (assetBlendKey.m_assetId.IsValid())
    {
        SyncValue(mv_loop, assetBlendKey.m_bLoop, false, pVar);
        SyncValue(mv_timeScale, assetBlendKey.m_speed, false, pVar);
        SyncValue(mv_blendInTime, assetBlendKey.m_blendInTime, false, pVar);
        SyncValue(mv_blendOutTime, assetBlendKey.m_blendOutTime, false, pVar);

        // Ask entity id this asset blend is bound to, in order to
        // get the duration of this asset in an asynchronous way.
        Maestro::SequenceComponentRequests::AnimatedFloatValue currValue = 0.0f;
        Maestro::SequenceComponentRequestBus::Event(pSequence->GetSequenceComponentEntityId(), &Maestro::SequenceComponentRequestBus::Events::GetAssetDuration,
            currValue, m_entityId, m_componentId, assetBlendKey.m_assetId );
        assetBlendKey.m_duration = currValue.GetFloatValue();
        assetBlendKey.m_startTime = 0.0f;
        assetBlendKey.m_endTime = assetBlendKey.m_duration;
        ResetStartEndLimits(assetBlendKey.m_duration);

        const auto speed = AZStd::clamp(assetBlendKey.m_speed, 0.01f, 100.0f);
        if (AZStd::abs(speed - assetBlendKey.m_speed) > AZ::Constants::FloatEpsilon)
        {
            mv_timeScale = assetBlendKey.m_speed = speed;
        }
    }
    else
    {
        // Zero-initialize the key value for an invalid motion asset
        mv_loop = assetBlendKey.m_bLoop = false;
        mv_timeScale = assetBlendKey.m_speed = 1.0f;
        mv_blendInTime = assetBlendKey.m_blendInTime = 0.0f;
        mv_blendOutTime = assetBlendKey.m_blendOutTime = 0.0f;
        assetBlendKey.m_duration = 0.0f;
        assetBlendKey.m_startTime = 0.0f;
        assetBlendKey.m_endTime = 0.0f;
    }

    keyHandle.SetKey(&assetBlendKey);
}
