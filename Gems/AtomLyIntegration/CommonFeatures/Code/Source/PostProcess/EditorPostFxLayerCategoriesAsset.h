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
#include <AzCore/std/containers/map.h>
#include <Atom/Feature/PostProcess/PostFxLayerCategoriesConstants.h>

namespace AZ
{
    namespace Render
    {
        class EditorPostFxLayerCategoriesAsset final
            : public AZ::Data::AssetData
        {
        public:
            AZ_RTTI(EditorPostFxLayerCategoriesAsset, "{A18B1B11-4C1E-4C1B-9643-178E8ED27019}", AZ::Data::AssetData);
            AZ_CLASS_ALLOCATOR(EditorPostFxLayerCategoriesAsset, AZ::SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* context);

            // layer categories for PostFX Layer Component indexed by priority
            PostFx::LayerCategoriesMap m_layerCategories;
        };
    }
}
