/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Image.h>
#include <Atom/RHI/ResourcePool.h>

namespace AZ::RHI
{
    //! A simple base class for multi-device image pools. This mainly exists so that various
    //! image pool implementations can have some type safety separate from other
    //! resource pool types.
    class ImagePoolBase : public ResourcePool
    {
    public:
        AZ_RTTI(ImagePoolBase, "{CAC2167A-D65A-493F-A450-FDE2B1A883B1}", ResourcePool);
        virtual ~ImagePoolBase() override = default;

    protected:
        ImagePoolBase() = default;

        ResultCode InitImage(Image* image, const ImageDescriptor& descriptor, PlatformMethod platformInitResourceMethod);

    private:
        using ResourcePool::InitResource;
    };
} // namespace AZ::RHI
