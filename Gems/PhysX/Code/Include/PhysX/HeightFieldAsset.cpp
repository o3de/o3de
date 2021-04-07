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

#include <PhysX_precompiled.h>

#include <PhysX/HeightFieldAsset.h>

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
            ReleaseMemory();

            m_heightField = heightField;
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
