/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Asset/WhiteBoxMeshAsset.h"
#include "Asset/WhiteBoxMeshAssetBus.h"

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/TickBus.h>
#include <IEditor.h>

namespace WhiteBox
{
    //! Handle creating, loading and setting White Box mesh assets.
    //! @note Used by the EditorWhiteBoxComponent to delegate asset handling responsibilities.
    class EditorWhiteBoxMeshAsset
        : private AZ::Data::AssetBus::Handler
        , private WhiteBoxMeshAssetNotificationBus::Handler
        , private IEditorNotifyListener
        , private AZ::TickBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ_TYPE_INFO(EditorWhiteBoxMeshAsset, "{4A9D9B10-9E60-4D59-A308-966829DC2B76}")
        static void Reflect(AZ::ReflectContext* context);

        EditorWhiteBoxMeshAsset() = default;
        ~EditorWhiteBoxMeshAsset();

        //! Release the stored WhiteBoxMeshAsset but retain the AssetId to be loaded again in future.
        void Release();
        //! Release the stored WhiteBoxMeshAsset and invalidate the AssetId to full clear the asset from use.
        //! @note m_entityComponentIdPair is not reset.
        void Reset();

        //! Associate an EditorWhiteBoxMeshAsset with a specific entity/component id pair that
        //! changes will be propagated to.
        void Associate(const AZ::EntityComponentIdPair& entityComponentIdPair);
        //! Transfer ownership of an in-memory White Box mesh and create an asset at the specified relative path.
        void TakeOwnershipOfWhiteBoxMesh(const AZStd::string& relativeAssetPath, Api::WhiteBoxMeshPtr whiteBoxMesh);
        //! Returns the WhiteBoxMesh stored on this asset.
        //! @note If an asset has not been set or loaded this value will be null.
        //! @note Do not hold onto a reference to this pointer for longer than a function call.
        WhiteBoxMesh* GetWhiteBoxMesh();
        //! Returns the AssetId of the White Box mesh asset.
        //! @note If an asset has not been set or loaded this value will be invalid.
        AZ::Data::AssetId GetWhiteBoxMeshAssetId() const;
        //! Returns the WhiteBoxMeshAsset.
        //! @note If an asset has not been set or loaded the asset will be empty/invalid.
        AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset> GetWhiteBoxMeshAsset();
        //! Helper to return if the White Box mesh asset id is valid or not (if it is valid, the asset is in use).
        bool InUse() const;

        //! Request a load of the stored asset.
        //! @note If the asset is not in use (asset id is invalid) the call will be a noop.
        void Load();
        //! Write the asset to disk at its existing location.
        void Save();
        //! Write the asset to disk at an arbitrary location.
        void Save(const AZStd::string& absolutePath);
        //! Returns if the asset is both set and loaded.
        //! @note An asset may be in use, but not yet loaded (InUse() != Loaded()).
        bool Loaded() const;
        //! Store the in-memory representation of the WhiteBoxMesh to the current WhiteBoxMeshAsset.
        void Serialize();

    private:
        // Edit context callbacks
        void AssetChanged();
        void AssetCleared();

        // AZ::Data::AssetBus ...
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloadError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        // WhiteBoxMeshAssetNotificationBus ...
        void OnWhiteBoxMeshAssetModified(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        // IEditorNotifyListener ...
        void OnEditorNotifyEvent(EEditorNotifyEvent editorEvent) override;

        //! Disconnect from buses/listeners before either releasing or destroying the asset.
        void Disconnect();

        // AZ::TickBus overrides ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // Listeners for legacy editor events when the level is saved.
        void RegisterForEditorEvents();
        void UnregisterForEditorEvents();

        //! A reference to White Box mesh data stored in an asset.
        AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset> m_meshAsset;
        AZ::EntityComponentIdPair m_entityComponentIdPair; //!< The entity/component this asset is associated with.
    };
} // namespace WhiteBox
