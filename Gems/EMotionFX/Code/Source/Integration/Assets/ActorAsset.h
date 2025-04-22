/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Asset/SimpleAsset.h>

#include <Integration/Assets/AssetCommon.h>
#include <Integration/Rendering/RenderActor.h>


namespace EMotionFX
{
    class Actor;
    class ActorInstance;

    namespace Integration
    {
        /**
         * Represents an EMotionFX actor asset.
         * Each asset maintains storage of the original EMotionFX binary asset (via EMotionFXAsset base class).
         * Initialization of the asset constructs Open 3D Engine rendering objects, such as the render mesh and material,
         * directly from the instantiated EMotionFX actor.
         * An easy future memory optimization is to wipe the EMotionFXAsset buffer after the actor, render meshes,
         * and materials are created, since it's technically no longer necessary. At this stage it's worth keeping
         * around for testing.
         */
        class ActorAsset
            : public EMotionFXAsset
        {
        public:
            friend class ActorAssetHandler;

            AZ_RTTI(ActorAsset, "{F67CC648-EA51-464C-9F5D-4A9CE41A7F86}", EMotionFXAsset)
            AZ_CLASS_ALLOCATOR_DECL

            ActorAsset(AZ::Data::AssetId id = AZ::Data::AssetId());

            using ActorInstancePtr = EMotionFXPtr<EMotionFX::ActorInstance>;
            ActorInstancePtr CreateInstance(AZ::Entity* entity);

            Actor* GetActor() const { return m_emfxActor.get(); }
            RenderActor* GetRenderActor() const { return m_renderActor.get(); }

            void SetData(AZStd::shared_ptr<Actor> actor);

            void InitRenderActor();

        private:
            AZStd::shared_ptr<Actor> m_emfxActor;
            AZStd::unique_ptr<RenderActor> m_renderActor;
        };

        /**
         * Asset handler for loading and initializing actor assets.
         * The OnInitAsset stage constructs Open 3D Engine render meshes and materials by extracting
         * said data from the EMotionFX actor.
         */
        class ActorAssetHandler
            : public EMotionFXAssetHandler<ActorAsset>
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL

            bool OnInitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
            AZ::Data::AssetType GetAssetType() const override;
            void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override;
            AZ::Uuid GetComponentTypeId() const override;
            const char* GetAssetTypeDisplayName() const override;
            const char* GetBrowserIcon() const override;
            int GetAssetTypeDragAndDropCreationPriority() const override;
        };
    } // namespace Integration

    using ActorAssetData = AZ::Data::Asset<Integration::ActorAsset>;
} // namespace EMotionFX

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::Integration::EMotionFXPtr<EMotionFX::Integration::ActorAsset>, "{3F60D391-F1C8-4A40-9946-A2637D088C48}");
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::Integration::EMotionFXPtr<EMotionFX::ActorInstance>, "{169ACF47-3DEF-482A-AB7D-4CC11934D932}");
}
