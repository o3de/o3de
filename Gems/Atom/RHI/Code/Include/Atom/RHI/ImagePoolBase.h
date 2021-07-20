/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ResourcePool.h>
#include <Atom/RHI/Image.h>

namespace AZ
{
    namespace RHI
    {
        /**
         * A simple base class for image pools. This mainly exists so that various
         * image pool implementations can have some type safety separate from other
         * resource pool types.
         */
        class ImagePoolBase
            : public ResourcePool
        {
        public:
            AZ_RTTI(ImagePoolBase, "{6353E390-C5D2-42FF-8AA9-9AFCD1F2F1B5}", ResourcePool);
            virtual ~ImagePoolBase() override = default;

        protected:
            ImagePoolBase() = default;

            ResultCode InitImage(
                Image* image,
                const ImageDescriptor& descriptor,
                PlatformMethod platformInitResourceMethod);

        private:
            using ResourcePool::InitResource;
        };
    }
}
