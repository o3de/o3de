/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace News
{
    class Resource;

    //! Descriptor is a simple solution to add additional functionality to a Resource
    /*!
        Some descriptors can only work with certain resource types, like AerticleDescriptor
    */
    class Descriptor
    {
    public:
        explicit Descriptor(Resource& resource);
        virtual ~Descriptor();

        Resource& GetResource() const
        {
            return m_resource;
        }

    protected:
        Resource& m_resource;
    };
}
