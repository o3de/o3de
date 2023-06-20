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
            AZ_CLASS_ALLOCATOR(EditorPostFxLayerCategoriesAsset, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* context);

            // layer categories for PostFX Layer Component indexed by priority
            PostFx::LayerCategoriesMap m_layerCategories;
        };
    }
}
