/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetTypeInfoBus.h>

namespace LmbrCentral
{
    /**
     * EntityIconAssetTypeInfo describes the Entity Icon Asset with which users can use
     * to customize entity icons. See \ref EditorEntityIconComponent for details.
     */
    class EntityIconAssetTypeInfo
        : public AZ::AssetTypeInfoBus::Handler
    {
    public:

        AZ_CLASS_ALLOCATOR(EntityIconAssetTypeInfo, AZ::SystemAllocator);

        ~EntityIconAssetTypeInfo() override;

        //////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::AssetTypeInfoBus::Handler
        AZ::Data::AssetType GetAssetType() const override;
        const char* GetAssetTypeDisplayName() const override;
        const char* GetGroup() const override;
        const char* GetBrowserIcon() const override;
        //////////////////////////////////////////////////////////////////////////////////////////////

        void Register();
        void Unregister();
    };
} // namespace LmbrCentral
