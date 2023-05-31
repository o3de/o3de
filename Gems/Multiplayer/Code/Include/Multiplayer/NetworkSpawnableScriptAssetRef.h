/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Spawnable/Script/SpawnableScriptAssetRef.h>
#include <Multiplayer/MultiplayerTypes.h>

namespace Multiplayer
{
    //! A wrapper around a Network Spawnable asset that can be used by Script Canvas and Lua.
    //! This is a subclass of the .spawnable asset reference that only allows .network.spawnable asset references.
    //! It exists to make scripts very explicit for when they're using the spawnable API with non-networked vs networked spawnables.
    class NetworkSpawnableScriptAssetRef
        : public AzFramework::Scripts::SpawnableScriptAssetRef
    {
    public:
        AZ_RTTI(NetworkSpawnableScriptAssetRef, "{2369101C-6C28-4F13-B918-896B37EAD988}", AzFramework::Scripts::SpawnableScriptAssetRef);

        static void Reflect(AZ::ReflectContext* context);

        NetworkSpawnableScriptAssetRef() = default;
        ~NetworkSpawnableScriptAssetRef() override = default;

    protected:
        // Override the asset selection so that we only can select *.network.spawnable files.
        bool ShowProductAssetFileName() const override;
        bool HideProductAssetFiles() const override;
        const char* GetAssetPickerTitle() const override;
        AZ::Outcome<void, AZStd::string> ValidatePotentialSpawnableAsset(void* newValue, const AZ::Uuid& valueType) const override;

    private:
        // Change the GetAsset / SetAsset script functions to specifically use Network spawnables and not just any spawnables.
        void SetAsset(NetworkSpawnable& asset);
        NetworkSpawnable GetAsset() const;

    };
}
