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
                    ->Version(1)
                    ;
            }
        }

        const Data::Asset<ResourcePoolAsset>& AttachmentImageAsset::GetPoolAsset() const
        {
            return m_poolAsset;
        }

        const RHI::ClearValue* AttachmentImageAsset::GetOptimizedClearValue() const
        {
            return m_isClearValueValid ? &m_optimizedClearValue : nullptr;
        }
    }
}
