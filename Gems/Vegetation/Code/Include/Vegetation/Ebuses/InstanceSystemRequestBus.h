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

#include <AzCore/Component/ComponentBus.h>
#include <Vegetation/Descriptor.h>

namespace Vegetation
{
    struct InstanceData;

    /**
    * An interface to manage creation and destruction of vegetation instances
    */
    class InstanceSystemRequests
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        ////////////////////////////////////////////////////////////////////////

        virtual ~InstanceSystemRequests()  = default;

        virtual DescriptorPtr RegisterUniqueDescriptor(const Descriptor& descriptor) = 0;
        virtual void ReleaseUniqueDescriptor(DescriptorPtr descriptorPtr) = 0;

        // create vegetation instance from description
        virtual void CreateInstance(InstanceData& instanceData) = 0;

        // destroy vegetation instance by id
        virtual void DestroyInstance(InstanceId instanceId) = 0;
        virtual void DestroyAllInstances() = 0;

        virtual void Cleanup() = 0;
    };

    using InstanceSystemRequestBus = AZ::EBus<InstanceSystemRequests>;

    /**
    * An interface to get statistics about vegetation instances
    */
    class InstanceSystemStatsRequests
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        ////////////////////////////////////////////////////////////////////////

        virtual ~InstanceSystemStatsRequests() = default;

        virtual AZ::u32 GetInstanceCount() const = 0;
        virtual AZ::u32 GetTotalTaskCount() const = 0;
        virtual AZ::u32 GetCreateTaskCount() const = 0;
        virtual AZ::u32 GetDestroyTaskCount() const = 0;
    };

    using InstanceSystemStatsRequestBus = AZ::EBus<InstanceSystemStatsRequests>;

} // namespace Vegetation
