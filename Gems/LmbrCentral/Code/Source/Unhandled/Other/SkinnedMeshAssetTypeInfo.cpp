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

#include "LmbrCentral_precompiled.h"
#include "SkinnedMeshAssetTypeInfo.h"

#include <LmbrCentral/Rendering/MaterialAsset.h>

namespace LmbrCentral
{
    static AZ::Data::AssetType skinnedMeshAssetType("{C5D443E1-41FF-4263-8654-9438BC888CB7}");

    SkinnedMeshAssetTypeInfo::~SkinnedMeshAssetTypeInfo()
    {
        Unregister();
    }

    void SkinnedMeshAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(skinnedMeshAssetType);
    }

    void SkinnedMeshAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(skinnedMeshAssetType);
    }

    AZ::Data::AssetType SkinnedMeshAssetTypeInfo::GetAssetType() const
    {
        return skinnedMeshAssetType;
    }

    const char* SkinnedMeshAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Skinned Mesh";
    }

    const char* SkinnedMeshAssetTypeInfo::GetGroup() const
    {
        return "Other";
    }
    const char * SkinnedMeshAssetTypeInfo::GetBrowserIcon() const
    {
        return "Editor/Icons/Components/SkinnedMesh.svg";
    }
} // namespace LmbrCentral
