/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Image/ImageAsset.h>
#include <Atom/RPI.Reflect/ResourcePoolAsset.h>

#include <Atom/RHI.Reflect/ClearValue.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! The asset for attachment image which is mainly used to create runtime attachment instance. 
        class ATOM_RPI_REFLECT_API AttachmentImageAsset final
            : public ImageAsset
        {
            friend class AttachmentImageAssetCreator;

        public:
            static constexpr const char* DisplayName{ "AttachmentImageAsset" };
            static constexpr const char* Group{ "Image" };
            static constexpr const char* Extension{ "attimage" };

            AZ_RTTI(AttachmentImageAsset, "{82CEA86B-E891-4969-8F35-D8017E8902C8}", ImageAsset);
            AZ_CLASS_ALLOCATOR(AttachmentImageAsset, AZ::SystemAllocator);
            static void Reflect(AZ::ReflectContext* context);

            ~AttachmentImageAsset() override = default;

            const Data::Asset<ResourcePoolAsset>& GetPoolAsset() const;

            //! Return the clear value of the image. The clear value may only be useful for certain type of images such as render targets (color/depth stencil). 
            const RHI::ClearValue* GetOptimizedClearValue() const;

            //! Return the name which can be used as debug name
            const AZ::Name& GetName() const;
            
            //! Return an unique name id which can be used as attachment id
            RHI::AttachmentId GetAttachmentId() const;

            //! Return ture if the attachment image has an unique name
            //! An attachment image with an unique name will be registered to image system
            //! and it can be found by ImageSystemInterface::FindRegisteredAttachmentImage() function
            //! The unique name is same as its attachment Id
            bool HasUniqueName() const;

        private:
            Data::Asset<ResourcePoolAsset> m_poolAsset;

            // an name id
            AZ::Name m_name;

            bool m_isUniqueName = false;

            // Clear value of the image
            AZStd::shared_ptr<RHI::ClearValue> m_optimizedClearValue;
        };

        using AttachmentImageAssetHandler = AssetHandler<AttachmentImageAsset>;
    }
}
