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

#include <Utils/AssetHelper.h>

#include <Utils/MeshAssetHelper.h>


namespace NvCloth
{
    const int InvalidIndex = -1;

    AssetHelper::AssetHelper(AZ::EntityId entityId)
        : m_entityId(entityId)
    {
    }

    AZStd::unique_ptr<AssetHelper> AssetHelper::CreateAssetHelper(AZ::EntityId entityId)
    {
        return entityId.IsValid()
            ? AZStd::make_unique<MeshAssetHelper>(entityId)
            : nullptr;
    }

    float AssetHelper::ConvertBackstopOffset(float backstopOffset)
    {
        constexpr float ToleranceU8 = 1.0f / 255.0f;

        // Convert range from [0,1] -> [-1,1]
        backstopOffset = AZ::GetClamp(backstopOffset * 2.0f - 1.0f, -1.0f, 1.0f);

        // Since the color was stored as U32 in the mesh, the low precision makes values
        // of 0.5f becoming an small negative number in the conversion from [0,1] to [-1,1].
        // So this sets the value to 0 when it is smaller than the tolerance of U8.
        backstopOffset = (std::fabs(backstopOffset) < ToleranceU8) ? 0.0f : backstopOffset;

        return backstopOffset;
    }
} // namespace NvCloth
