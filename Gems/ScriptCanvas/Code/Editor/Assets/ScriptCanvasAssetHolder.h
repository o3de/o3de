/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Asset/AssetCommon.h>
#include <Editor/Assets/ScriptCanvasAssetTrackerBus.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvasEditor
{
    class ScriptCanvasAsset;

    /*! ScriptCanvasAssetHolder
    Wraps a ScriptCanvasAsset reference and registers for the individual AssetBus events
    for saving, loading and unloading the asset.
    The ScriptCanvasAsset Holder contains functionality for activating the ScriptCanvasEntity stored on the reference asset
    as well as attempting to open the ScriptCanvasAsset within the ScriptCanvas Editor.
    It also provides the EditContext reflection for opening the asset in the ScriptCanvas Editor via a button
    */
    class ScriptCanvasAssetHolder
        : AssetTrackerNotificationBus::Handler
        , AZ::Data::AssetBus::Handler
    {
    public:
        AZ_RTTI(ScriptCanvasAssetHolder, "{3E80CEE3-2932-4DC1-AADF-398FDDC6DEFE}");
        AZ_CLASS_ALLOCATOR(ScriptCanvasAssetHolder, AZ::SystemAllocator, 0);

        using ScriptChangedCB = AZStd::function<void(AZ::Data::AssetId)>;

        ScriptCanvasAssetHolder() = default;
        ~ScriptCanvasAssetHolder() override;
        
        static void Reflect(AZ::ReflectContext* context);

        void Init(AZ::EntityId ownerId = AZ::EntityId(), AZ::ComponentId componentId = AZ::ComponentId());

        const AZ::Data::AssetType& GetAssetType() const;
        void ClearAsset();

        void SetAsset(AZ::Data::AssetId fileAssetId);
        AZ::Data::AssetId GetAssetId() const;

        ScriptCanvas::ScriptCanvasId GetScriptCanvasId() const;

        void LaunchScriptCanvasEditor(const AZ::Data::AssetId&, const AZ::Data::AssetType&) const;
        void OpenEditor() const;

        void SetScriptChangedCB(const ScriptChangedCB&);
        void Load(AZ::Data::AssetId fileAssetId);
        void LoadMemoryAsset(AZ::Data::AssetId fileAssetId);

        //! AZ::Data::AssetBus
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        ////

        const AZStd::string_view GetAssetHint() const
        {
            if (m_scriptCanvasAsset)
            {
                return m_scriptCanvasAsset.GetHint().c_str();
            }

            if (m_memoryScriptCanvasAsset)
            {
                return m_memoryScriptCanvasAsset.GetHint().c_str();
            }

            return "";
        }

    protected:

        //=====================================================================
        // AssetTrackerNotificationBus
        void OnAssetReady(const ScriptCanvasMemoryAsset::pointer asset) override;
        //=====================================================================

        //! Reloads the Script From the AssetData if it has changed
        AZ::u32 OnScriptChanged();
        
        AZ::Data::Asset<ScriptCanvasAsset> m_scriptCanvasAsset;
        AZ::Data::Asset<ScriptCanvasAsset> m_memoryScriptCanvasAsset;

        TypeDefs::EntityComponentId m_ownerId; // Id of Entity which stores this AssetHolder object
        ScriptChangedCB m_scriptNotifyCallback;

        bool m_triggeredLoad = false;
    };

}
