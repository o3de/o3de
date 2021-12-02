/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>

#include <Integration/Assets/AnimGraphAsset.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/AnimGraph.h>

namespace EMotionFX
{
    namespace Integration
    {
        AZ_CLASS_ALLOCATOR_IMPL(AnimGraphAsset, EMotionFXAllocator, 0)
        AZ_CLASS_ALLOCATOR_IMPL(AnimGraphAssetHandler, EMotionFXAllocator, 0)

        AnimGraphAsset::AnimGraphAsset(AZ::Data::AssetId id)
            : EMotionFXAsset(id)
        {}

        AnimGraphAsset::AnimGraphInstancePtr AnimGraphAsset::CreateInstance(
            EMotionFX::ActorInstance* actorInstance,
            EMotionFX::MotionSet* motionSet)
        {
            AZ_Assert(m_emfxAnimGraph, "Anim graph asset is not loaded");
            auto animGraphInstance = EMotionFXPtr<EMotionFX::AnimGraphInstance>::MakeFromNew(
                EMotionFX::AnimGraphInstance::Create(m_emfxAnimGraph.get(), actorInstance, motionSet));

            if (animGraphInstance)
            {
                animGraphInstance->SetIsOwnedByRuntime(true);
            }

            return animGraphInstance;
        }

        void AnimGraphAsset::SetData(EMotionFX::AnimGraph* animGraph)
        {
            m_emfxAnimGraph.reset(animGraph);
            m_status = AZ::Data::AssetData::AssetStatus::Ready;
        }

        //////////////////////////////////////////////////////////////////////////
        bool AnimGraphAssetHandler::OnInitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            AnimGraphAsset* assetData = asset.GetAs<AnimGraphAsset>();
            assetData->m_emfxAnimGraph.reset(EMotionFX::GetImporter().LoadAnimGraph(
                assetData->m_emfxNativeData.data(),
                assetData->m_emfxNativeData.size()));

            if (assetData->m_emfxAnimGraph)
            {
                assetData->m_emfxAnimGraph->SetIsOwnedByAsset(true);
                assetData->m_emfxAnimGraph->SetIsOwnedByRuntime(true);

                assetData->m_emfxAnimGraph->FindAndRemoveCycles();

                // The following code is required to be set so the FileManager detects changes to the files loaded
                // through this method. Once EMotionFX is integrated to the asset system this can go away.
                AZStd::string assetFilename;
                EBUS_EVENT_RESULT(assetFilename, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, asset.GetId());
                AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
                if (!projectPath.empty())
                {
                    AZ::IO::FixedMaxPathString filename{ (projectPath / assetFilename).LexicallyNormal().FixedMaxPathStringAsPosix() };

                    assetData->m_emfxAnimGraph->SetFileName(filename.c_str());
                }
                else
                {
                    if (GetEMotionFX().GetIsInEditorMode())
                    {
                        AZ_Warning("EMotionFX", false, "Failed to retrieve project root path . Cannot set absolute filename for '%s'", assetFilename.c_str());
                    }
                    assetData->m_emfxAnimGraph->SetFileName(assetFilename.c_str());
                }
            }

            assetData->ReleaseEMotionFXData();
            AZ_Error("EMotionFX", assetData->m_emfxAnimGraph, "Failed to initialize anim graph asset %s", asset.GetHint().c_str());
            return static_cast<bool>(assetData->m_emfxAnimGraph);
        }


        void AnimGraphAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
        {
            AnimGraphAsset* animGraphAsset = static_cast<AnimGraphAsset*>(ptr);
            EMotionFX::AnimGraph* animGraph = animGraphAsset->GetAnimGraph();

            if (animGraph)
            {
                // Get rid of all anim graph instances that refer to the anim graph we're about to destroy.
                EMotionFX::GetAnimGraphManager().RemoveAnimGraphInstances(animGraph);
            }

            delete ptr;
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Data::AssetType AnimGraphAssetHandler::GetAssetType() const
        {
            return azrtti_typeid<AnimGraphAsset>();
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
        {
            extensions.push_back("animgraph");
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Uuid AnimGraphAssetHandler::GetComponentTypeId() const
        {
            // The system is not in place to allow for components to drive creation of required components
            // Future work will enable this functionality which will allow dropping in viewport or Inspector panel
            // EditorAnimGraphComponent
            // return AZ::Uuid("{770F0A71-59EA-413B-8DAB-235FB0FF1384}");
            
            // Returning null keeps the animgraph from being drag/dropped
            return AZ::Uuid::CreateNull();
        }

        //////////////////////////////////////////////////////////////////////////
        const char* AnimGraphAssetHandler::GetAssetTypeDisplayName() const
        {
            return "EMotion FX Anim Graph";
        }

        const char* AnimGraphAssetHandler::GetBrowserIcon() const
        {
            return "Editor/Images/AssetBrowser/AnimGraph_16.svg";
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphAssetBuilderHandler::InitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset, bool loadStageSucceeded, bool isReload)
        {
            // Don't need to load the referenced animpgraph asset since we only care about the product ID or relative path of the product dependency
            AZ_UNUSED(asset);
            AZ_UNUSED(loadStageSucceeded);
            AZ_UNUSED(isReload);
        }

        AZ::Data::AssetHandler::LoadResult AnimGraphAssetBuilderHandler::LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            AZ_UNUSED(asset);
            AZ_UNUSED(stream);
            AZ_UNUSED(assetLoadFilterCB);

            return AZ::Data::AssetHandler::LoadResult::LoadComplete;
        }

    } // namespace Integration
} // namespace EMotionFX
