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
#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include <smartptr.h>
#include <IStatObj.h>

namespace LmbrCentral
{
    class MeshAsset
        : public AZ::Data::AssetData
    {
    public:
        using MeshPtr = _smart_ptr<IStatObj>;

        AZ_RTTI(MeshAsset, "{C2869E3B-DDA0-4E01-8FE3-6770D788866B}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(MeshAsset, AZ::SystemAllocator, 0);

        /// The assigned static mesh instance.
        MeshPtr m_statObj = nullptr;
    };

    // for "character definition files"
    class CharacterDefinitionAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(CharacterDefinitionAsset, "{DF036C63-9AE6-4AC3-A6AC-8A1D76126C01}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(CharacterDefinitionAsset, AZ::SystemAllocator, 0);

        /// Character assets are unique per load.
        bool IsRegisterReadonlyAndShareable() override
        {
            return false;
        }
    };

    // note:  Skinned Mesh assets, characters, or other similar files (source is .skin, .chr, or fbx files) which contain geometry
    // will have a different UUID to the above CDF.  
    // Currently .SKIN assigned as "{C5D443E1-41FF-4263-8654-9438BC888CB7}" in the AP and its LODs are "{58E5824F-C27B-46FD-AD48-865BA41B7A51}".
    // with the lod bits set in the SUBID
    // note that there is no current reserved UUID for .chr files.

} // namespace LmbrCentral
