/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PhysX/HeightFieldAsset.h>
#include <PxPhysicsAPI.h>

namespace PhysX
{
    namespace Pipeline
    {
        HeightFieldAsset::~HeightFieldAsset()
        {
            ReleaseMemory();
        }

        physx::PxHeightField* HeightFieldAsset::GetHeightField()
        {
            return m_heightField;
        }

        const physx::PxHeightField* HeightFieldAsset::GetHeightField() const
        {
            return m_heightField;
        }

        void HeightFieldAsset::SetHeightField(physx::PxHeightField* heightField)
        {
            if (heightField != m_heightField)
            {
                ReleaseMemory();
                m_heightField = heightField;
            }
            m_status = AssetStatus::Ready;
        }

        float HeightFieldAsset::GetMinHeight() const
        {
            return m_minHeight;
        }

        void HeightFieldAsset::SetMinHeight(float height)
        {
            m_minHeight = height;
        }

        float HeightFieldAsset::GetMaxHeight() const
        {
            return m_maxHeight;
        }

        void HeightFieldAsset::SetMaxHeight(float height)
        {
            m_maxHeight = height;
        }

        void HeightFieldAsset::ReleaseMemory()
        {
            if (m_heightField)
            {
                m_heightField->release();
                m_heightField = nullptr;
            }
        }
    } // namespace Pipeline
} // namespace PhysX
