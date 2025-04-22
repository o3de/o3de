/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>

namespace AZ
{
    namespace RPI
    {
        void AttachmentImageAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<AttachmentImageAsset, ImageAsset>()
                    ->Version(2)
                    ->Field("Name", &AttachmentImageAsset::m_name)
                    ->Field("IsUniqueName", &AttachmentImageAsset::m_isUniqueName)
                    ->Field("OptimizedClearValue", &AttachmentImageAsset::m_optimizedClearValue)
                    ;
            }
        }

        const Data::Asset<ResourcePoolAsset>& AttachmentImageAsset::GetPoolAsset() const
        {
            return m_poolAsset;
        }

        const RHI::ClearValue* AttachmentImageAsset::GetOptimizedClearValue() const
        {
            return m_optimizedClearValue.get();
        }

        const AZ::Name& AttachmentImageAsset::GetName() const
        {
            return m_name;
        }

        RHI::AttachmentId AttachmentImageAsset::GetAttachmentId() const
        {
            if (HasUniqueName())
            {
                return m_name;
            }
            return Name(m_assetId.ToString<AZStd::string>());
        }

         bool AttachmentImageAsset::HasUniqueName() const
        {
             // The name can still be empty if the asset was loaded for data file but not from AttachmentImageAssetCreator
            return m_isUniqueName && !m_name.IsEmpty();
        }
    }
}
