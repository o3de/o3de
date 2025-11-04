/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzFramework/Asset/GenericAssetHandler.h>

namespace physx
{
    class PxHeightField;
}

namespace PhysX
{
    namespace Pipeline
    {

        /// Represents a PhysX height field asset.
        class HeightFieldAsset final
            : public AZ::Data::AssetData
        {
        public:
            friend class HeightFieldAssetHandler;

            AZ_CLASS_ALLOCATOR(HeightFieldAsset, AZ::SystemAllocator);
            AZ_RTTI(HeightFieldAsset, "{B61189FE-B2D7-4AF1-8951-CB5C0F7834FC}", AZ::Data::AssetData);

            ~HeightFieldAsset();

            physx::PxHeightField* GetHeightField();

            const physx::PxHeightField* GetHeightField() const;

            void SetHeightField(physx::PxHeightField* heightField);

            float GetMinHeight() const;
            void SetMinHeight(float height);
            float GetMaxHeight() const;
            void SetMaxHeight(float height);

        private:
            void ReleaseMemory();

            physx::PxHeightField* m_heightField = nullptr;
            float m_minHeight = 0.0f;
            float m_maxHeight = 0.0f;
        };
    } // namespace Pipeline
} // namespace PhysX
