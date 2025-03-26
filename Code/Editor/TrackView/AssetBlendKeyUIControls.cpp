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
    const float time_zero = .0f;
    float step = ReflectedPropertyItem::ComputeSliderStep(time_zero, assetBlendKeyDuration);
    mv_startTime.GetVar()->SetLimits(time_zero, assetBlendKeyDuration, step, true, true);
    mv_endTime.GetVar()->SetLimits(time_zero, assetBlendKeyDuration, step, true, true);
    mv_blendInTime.GetVar()->SetLimits(time_zero, assetBlendKeyDuration, step, true, true);
    mv_blendOutTime.GetVar()->SetLimits(time_zero, assetBlendKeyDuration, step, true, true);
}

bool CAssetBlendKeyUIControls::OnKeySelectionChange(const CTrackViewKeyBundle& selectedKeys)
{
    if (!selectedKeys.AreAllKeysOfSameType())
    {
        return false;
    }

    bool bAssigned = false;
    if (selectedKeys.GetKeyCount() == 1)
    {
        const CTrackViewKeyHandle& keyHandle = selectedKeys.GetKey(0);
        if (keyHandle.GetTrack()->GetValueType() == AnimValueType::AssetBlend)
        {
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

            mv_asset->SetUserData(assetBlendKey.m_assetId.m_subId);
            mv_asset->SetDisplayValue(assetBlendKey.m_assetId.m_guid.ToString<AZStd::string>().c_str());
            mv_loop = assetBlendKey.m_bLoop;
            mv_endTime = assetBlendKey.m_endTime;
            mv_startTime = assetBlendKey.m_startTime;
            mv_timeScale = assetBlendKey.m_speed;
            mv_blendInTime = assetBlendKey.m_blendInTime;
            mv_blendOutTime = assetBlendKey.m_blendOutTime;

            bAssigned = true;
        }
    }

    return bAssigned;
}

// Called when UI variable changes.
void CAssetBlendKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
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

        if (keyHandle.GetTrack()->GetValueType() == AnimValueType::AssetBlend)
        {
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

            SyncValue(mv_loop, assetBlendKey.m_bLoop, false, pVar);
            SyncValue(mv_startTime, assetBlendKey.m_startTime, false, pVar);
            SyncValue(mv_endTime, assetBlendKey.m_endTime, false, pVar);
            SyncValue(mv_timeScale, assetBlendKey.m_speed, false, pVar);
            SyncValue(mv_blendInTime, assetBlendKey.m_blendInTime, false, pVar);
            SyncValue(mv_blendOutTime, assetBlendKey.m_blendOutTime, false, pVar);

            if (assetBlendKey.m_assetId.IsValid())
            {
                // Ask entity id this asset blend is bound to, to
                // get the duration of this asset in an async way.
                Maestro::SequenceComponentRequests::AnimatedFloatValue currValue = 0.0f;
                Maestro::SequenceComponentRequestBus::Event(
                    pSequence->GetSequenceComponentEntityId(),
                    &Maestro::SequenceComponentRequestBus::Events::GetAssetDuration,
                    currValue,
                    m_entityId,
                    m_componentId,
                    assetBlendKey.m_assetId
                );

                assetBlendKey.m_duration = currValue.GetFloatValue();
                ResetStartEndLimits(assetBlendKey.m_duration);
            }

            keyHandle.SetKey(&assetBlendKey);
        }
    }
}
