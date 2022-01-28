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
        const char* AttachmentImageAsset::DisplayName = "AttachmentImageAsset";
        const char* AttachmentImageAsset::Group = "Image";
        const char* AttachmentImageAsset::Extension = "attimage";

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

        RHI::AttachmentId AttachmentImageAsset::GeAttachmentId() const
        {
            if (m_isUniqueName)
            {
                if (!m_name.IsEmpty())
                {
                    return m_name;
                }
            }
            return Name(m_assetId.ToString<AZStd::string>());
        }

         bool AttachmentImageAsset::HasUniqueName() const
        {
            return m_isUniqueName;
        }
    }
}
