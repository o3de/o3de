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