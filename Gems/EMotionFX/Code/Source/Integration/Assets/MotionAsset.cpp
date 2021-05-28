/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/


#include "EMotionFX_precompiled.h"

#include <Integration/Assets/MotionAsset.h>

namespace EMotionFX
{
    namespace Integration
    {
        AZ_CLASS_ALLOCATOR_IMPL(MotionAsset, EMotionFXAllocator, 0);
        AZ_CLASS_ALLOCATOR_IMPL(MotionAssetHandler, EMotionFXAllocator, 0);

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
