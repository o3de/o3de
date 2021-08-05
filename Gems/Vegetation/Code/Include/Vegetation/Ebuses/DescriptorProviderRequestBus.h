/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/containers/vector.h>
#include <Vegetation/Descriptor.h>

namespace Vegetation
{
    /**
    * the EBus is used to request vegetation descriptors from a provider
    */
    class DescriptorProviderRequests : public AZ::ComponentBus
    {
    public:
        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;

        virtual void GetDescriptors(DescriptorPtrVec& descriptors) const = 0;
    };

    typedef AZ::EBus<DescriptorProviderRequests> DescriptorProviderRequestBus;
}
