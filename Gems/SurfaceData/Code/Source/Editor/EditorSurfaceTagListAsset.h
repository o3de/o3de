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
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    class ReflectContext;
}

namespace SurfaceData
{
    /**
    * Asset containing dictionary of known tags
    */
    class EditorSurfaceTagListAsset final
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(EditorSurfaceTagListAsset, "{A471B2A9-85FC-4993-842D-1881CBC03A2B}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(EditorSurfaceTagListAsset, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);
    
        AZStd::vector<AZStd::string> m_surfaceTagNames;
    };

} // namespace SurfaceData