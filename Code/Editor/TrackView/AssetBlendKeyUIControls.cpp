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
#include <CryCommon/Maestro/Types/AnimNodeType.h>
#include <CryCommon/Maestro/Types/AnimValueType.h>
#include <CryCommon/Maestro/Bus/SequenceComponentBus.h>
#include <CryCommon/Maestro/Types/AssetBlendKey.h>

// Editor
#include "TrackViewKeyPropertiesDlg.h"
#include "Controls/ReflectedPropertyControl/ReflectedPropertyItem.h"


//////////////////////////////////////////////////////////////////////////
class CAssetBlendKeyUIControls
    : public CTrackViewKeyUIControls
{
public:

    AZ::EntityId m_entityId;
    AZ::ComponentId m_componentId;

    CSmartVariableArray mv_table;
    CSmartVariable<QString> mv_asset;
    CSmartVariable<bool> mv_loop;
    CSmartVariable<float> mv_startTime;
    CSmartVariable<float> mv_endTime;
    CSmartVariable<float> mv_timeScale;
    CSmartVariable<float> mv_blendInTime;
    CSmartVariable<float> mv_blendOutTime;

    void OnCreateVars() override
    {
        // Init to an invalid id
        AZ::Data::AssetId assetId;
        assetId.SetInvalid();
        mv_asset->SetUserData(assetId.m_subId);
        mv_asset->SetDisplayValue(assetId.m_guid.ToString<AZStd::string>().c_str());

        AddVariable(mv_table, "Key Properties");
        // In the future, we may have different types of AssetBlends supported. Right now
        // "motion" for the Simple Motion Component is the only instance.
        AddVariable(mv_table, mv_asset, "Motion", IVariable::DT_MOTION);
        AddVariable(mv_table, mv_loop, "Loop");
        AddVariable(mv_table, mv_startTime, "Start Time");
        AddVariable(mv_table, mv_endTime, "End Time");
        AddVariable(mv_table, mv_timeScale, "Time Scale");
        AddVariable(mv_table, mv_blendInTime, "Blend In Time");
        AddVariable(mv_table, mv_blendOutTime, "Blend Out Time");
        mv_timeScale->SetLimits(0.001f, 100.f);
    }

    bool SupportTrackType([[maybe_unused]] const CAnimParamType& paramType, [[maybe_unused]] EAnimCurveType trackType, AnimValueType valueType) const override
    {
        return valueType == AnimValueType::AssetBlend;
    }

    bool OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys) override;
    void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys) override;

    unsigned int GetPriority() const override { return 1; }

    static const GUID& GetClassID()
    {
        // {5DC82D28-6C50-4406-8993-06770C640F98}
        static const GUID guid =
        {
            0x5DC82D28, 0x6C50, 0x4406, { 0x89, 0x93, 0x06, 0x77, 0x0C, 0x64, 0x0F, 0x98 }
        };
        return guid;
    }

protected:
    void ResetStartEndLimits(float AssetBlendKeyDuration);
};

//////////////////////////////////////////////////////////////////////////
void CAssetBlendKeyUIControls::ResetStartEndLimits(float assetBlendKeyDuration)
{
    const float time_zero = .0f;
    float step = ReflectedPropertyItem::ComputeSliderStep(time_zero, assetBlendKeyDuration);
    mv_startTime.GetVar()->SetLimits(time_zero, assetBlendKeyDuration, step, true, true);
    mv_endTime.GetVar()->SetLimits(time_zero, assetBlendKeyDuration, step, true, true);
    mv_blendInTime.GetVar()->SetLimits(time_zero, assetBlendKeyDuration, step, true, true);
    mv_blendOutTime.GetVar()->SetLimits(time_zero, assetBlendKeyDuration, step, true, true);
}

bool CAssetBlendKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys)
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
                    EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, assetBlendKey.m_assetId);

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

REGISTER_QT_CLASS_DESC(CAssetBlendKeyUIControls, "TrackView.KeyUI.AssetBlends", "TrackViewKeyUI");
