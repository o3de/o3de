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

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Image/ImageAsset.h>
#include <Atom/RPI.Reflect/ResourcePoolAsset.h>

#include <Atom/RHI.Reflect/ClearValue.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! The asset for attachment image which is mainly used to create runtime attachment instance. 
        class AttachmentImageAsset final
            : public ImageAsset
        {
            friend class AttachmentImageAssetCreator;

        public:
            static const char* DisplayName;
            static const char* Group;
            static const char* Extension;

            AZ_RTTI(AttachmentImageAsset, "{82CEA86B-E891-4969-8F35-D8017E8902C8}", ImageAsset);
            AZ_CLASS_ALLOCATOR(AttachmentImageAsset, AZ::SystemAllocator, 0);
            static void Reflect(AZ::ReflectContext* context);

            ~AttachmentImageAsset() override = default;

            const Data::Asset<ResourcePoolAsset>& GetPoolAsset() const;

            //! Return the clear value of the image. The clear value may only be useful for certain type of images such as render targets (color/depth stencil). 
            const RHI::ClearValue* GetOptimizedClearValue() const;

        private:
            Data::Asset<ResourcePoolAsset> m_poolAsset;

            // Clear value of the image. The value is only valid when m_isClearValueValid is true
            RHI::ClearValue m_optimizedClearValue;
            bool m_isClearValueValid = false;
        };

        using AttachmentImageAssetHandler = AssetHandler<AttachmentImageAsset>;
    }
}
