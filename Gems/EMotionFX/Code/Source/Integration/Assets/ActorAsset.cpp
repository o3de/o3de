/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <Integration/Assets/ActorAsset.h>
#include <Integration/Rendering/RenderBackendManager.h>


namespace EMotionFX
{
    namespace Integration
    {
        AZ_CLASS_ALLOCATOR_IMPL(ActorAsset, EMotionFXAllocator, 0)
        AZ_CLASS_ALLOCATOR_IMPL(ActorAssetHandler, EMotionFXAllocator, 0)

        ActorAsset::ActorAsset(AZ::Data::AssetId id)
            : EMotionFXAsset(id)
        {}

        ActorAsset::ActorInstancePtr ActorAsset::CreateInstance(AZ::Entity* entity)
        {
            AZ_Assert(m_emfxActor, "Actor asset is not loaded");
            ActorInstancePtr actorInstance = ActorInstancePtr::MakeFromNew(EMotionFX::ActorInstance::Create(m_emfxActor.get(), entity));
            if (actorInstance)
            {
                actorInstance->SetIsOwnedByRuntime(true);
            }
            return actorInstance;
        }

        void ActorAsset::SetData(AZStd::shared_ptr<Actor> actor)
        {
            m_emfxActor = AZStd::move(actor);
            m_status = AZ::Data::AssetData::AssetStatus::Ready;
        }

        void ActorAsset::InitRenderActor()
        {
            RenderBackend* renderBackend = AZ::Interface<RenderBackendManager>::Get()->GetRenderBackend();
            m_renderActor.reset(renderBackend->CreateActor(this));
        }

        bool ActorAssetHandler::OnInitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            ActorAsset* assetData = asset.GetAs<ActorAsset>();

            Importer::ActorSettings actorSettings;
            if (GetEMotionFX().GetEnableServerOptimization())
            {
                actorSettings.m_optimizeForServer = true;
            }

            assetData->m_emfxActor = EMotionFX::GetImporter().LoadActor(
                assetData->m_emfxNativeData.data(),
                assetData->m_emfxNativeData.size(),
                &actorSettings,
                "");

            assetData->m_emfxActor->SetFileName(asset.GetHint().c_str());
            assetData->m_emfxActor->Finalize();

            // Clear out the EMFX raw asset data.
            assetData->ReleaseEMotionFXData();

            if (!assetData->m_emfxActor)
            {
                AZ_Error("EMotionFX", false, "Failed to initialize actor asset %s", asset.ToString<AZStd::string>().c_str());
                return false;
            }

            // Note: Render actor depends on the mesh asset, so we need to manually create it after mesh asset has been loaded.
            return static_cast<bool>(assetData->m_emfxActor);
        }

        AZ::Data::AssetType ActorAssetHandler::GetAssetType() const
        {
            return azrtti_typeid<ActorAsset>();
        }

        void ActorAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
        {
            extensions.push_back("actor");
        }

        AZ::Uuid ActorAssetHandler::GetComponentTypeId() const
        {
            // EditorActorComponent
            return AZ::Uuid("{A863EE1B-8CFD-4EDD-BA0D-1CEC2879AD44}");
        }

        const char* ActorAssetHandler::GetAssetTypeDisplayName() const
        {
            return "EMotion FX Actor";
        }

        const char* ActorAssetHandler::GetBrowserIcon() const
        {
            return "Editor/Images/AssetBrowser/Actor_16.svg";
        }
    } //namespace Integration
} // namespace EMotionFX
