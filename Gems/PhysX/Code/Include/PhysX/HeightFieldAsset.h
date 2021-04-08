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

namespace physx
{
    class PxHeightField;
}

namespace PhysX
{
    namespace Pipeline
    {

        /// Represents a PhysX height field asset.
        class HeightFieldAsset
            : public AZ::Data::AssetData
        {
        public:
            friend class HeightFieldAssetHandler;

            AZ_CLASS_ALLOCATOR(HeightFieldAsset, AZ::SystemAllocator, 0);
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
