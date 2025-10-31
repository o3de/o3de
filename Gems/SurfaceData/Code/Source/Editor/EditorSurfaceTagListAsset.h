/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(EditorSurfaceTagListAsset, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);
    
        AZStd::vector<AZStd::string> m_surfaceTagNames;
    };

} // namespace SurfaceData
