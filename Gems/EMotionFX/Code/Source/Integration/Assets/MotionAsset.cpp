/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Integration/Assets/MotionAsset.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/Motion.h>

namespace EMotionFX
{
    namespace Integration
    {
        AZ_CLASS_ALLOCATOR_IMPL(MotionAsset, EMotionFXAllocator);
        AZ_CLASS_ALLOCATOR_IMPL(MotionAssetHandler, EMotionFXAllocator);

        MotionAsset::MotionAsset(AZ::Data::AssetId id)
            : EMotionFXAsset(id)
        {}

        //////////////////////////////////////////////////////////////////////////
        void MotionAsset::SetData(EMotionFX::Motion* motion)
        {
            m_emfxMotion.reset(motion);
            m_status = AZ::Data::AssetData::AssetStatus::Ready;
        }

        //////////////////////////////////////////////////////////////////////////
        bool MotionAssetHandler::OnInitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            MotionAsset* assetData = asset.GetAs<MotionAsset>();
            assetData->m_emfxMotion = EMotionFXPtr<EMotionFX::Motion>::MakeFromNew(EMotionFX::GetImporter().LoadMotion(
                assetData->m_emfxNativeData.data(),
                assetData->m_emfxNativeData.size(),
                nullptr));

            if (assetData->m_emfxMotion)
            {
                assetData->m_emfxMotion->SetIsOwnedByRuntime(true);
            }

            assetData->ReleaseEMotionFXData();
            AZ_Error("EMotionFX", assetData->m_emfxMotion, "Failed to initialize motion asset %s", asset.GetHint().c_str());
            return (assetData->m_emfxMotion);
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Data::AssetType MotionAssetHandler::GetAssetType() const
        {
            return azrtti_typeid<MotionAsset>();
        }

        //////////////////////////////////////////////////////////////////////////
        void MotionAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
        {
            extensions.push_back("motion");
        }

        //////////////////////////////////////////////////////////////////////////
        const char* MotionAssetHandler::GetAssetTypeDisplayName() const
        {
            return "EMotion FX Motion";
        }

        //////////////////////////////////////////////////////////////////////////
        const char* MotionAssetHandler::GetBrowserIcon() const
        {
            return "Editor/Images/AssetBrowser/Motion_16.svg";
        }

    } // namespace Integration
} // namespace EMotionFX
