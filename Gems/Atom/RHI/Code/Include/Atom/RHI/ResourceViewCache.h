/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 #pragma once

#include <AzCore/Utils/TypeHash.h>
#include <Atom/RHI.Reflect/Base.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/containers/unordered_map.h>

 namespace AZ::RHI
{
    class DeviceResourceView;
    struct ImageViewDescriptor;
    struct BufferViewDescriptor;
    class DeviceImageView;
    class DeviceBufferView;
    class DeviceResource;
    class Resource;
    class ResourceView;
    class ImageView;
    class BufferView;

    namespace ResourceViewCacheHelper
    {
        Ptr<DeviceImageView> CreateView(const DeviceResource* resource, const ImageViewDescriptor& imageViewDescriptor);
        Ptr<DeviceBufferView> CreateView(const DeviceResource* resource, const BufferViewDescriptor& bufferViewDescriptor);
        Ptr<ImageView> CreateView(const Resource* resource, const ImageViewDescriptor& imageViewDescriptor);
        Ptr<BufferView> CreateView(const Resource* resource, const BufferViewDescriptor& bufferViewDescriptor);

        //! Helper struct to provide correct Type and creation method for (Device)Image/Bufferview
        template<typename Type, typename DescriptorType>
        struct ResourceViewTypeHelper;

        template<>
        struct ResourceViewTypeHelper<DeviceResource, ImageViewDescriptor>
        {
            using ViewType = DeviceImageView;
        };

        template<>
        struct ResourceViewTypeHelper<DeviceResource, BufferViewDescriptor>
        {
            using ViewType = DeviceBufferView;
        };

        template<>
        struct ResourceViewTypeHelper<Resource, ImageViewDescriptor>
        {
            using ViewType = ImageView;
        };

        template<>
        struct ResourceViewTypeHelper<Resource, BufferViewDescriptor>
        {
            using ViewType = BufferView;
        };
    } // namespace ResourceViewCacheHelper

    //! ResourceViewCache is used by both Resource and DeviceResource to cache raw pointers to ResourceView and DeviceResourceView
    //! respectively. As ResourceViewType has a strong dependency (by holding a ConstrPtr) to ResourceType, this cache holds raw pointers to
    //! ensure no circular dependency arises.
    //! As it can be accessed in parallel, access to the cache is protected by a mutex.
    template<typename ResourceType>
    struct ResourceViewCache
    {
        //! Helper struct to select (Device)ResourceView based on (Device)Resource and the corresponding views for Buffers and Images
        template <typename Type>
        struct ResourceTypeHelper;

        template <>
        struct ResourceTypeHelper<DeviceResource>
        {
            using ResourceViewType = DeviceResourceView;
            template<typename DescriptorType>
            using ViewType = typename ResourceViewCacheHelper::ResourceViewTypeHelper<DeviceResource, DescriptorType>::ViewType;
        };

        template <>
        struct ResourceTypeHelper<Resource>
        {
            using ResourceViewType = ResourceView;
            template<typename DescriptorType>
            using ViewType = typename ResourceViewCacheHelper::ResourceViewTypeHelper<Resource, DescriptorType>::ViewType;
        };

        //! Returns true if the ResourceViewType is in the cache
        template <typename DescriptorType>
        bool IsInResourceCache(const DescriptorType& viewDescriptor);

        //! Removes the provided ResourceViewType from the cache
        void EraseResourceView(typename ResourceTypeHelper<ResourceType>::ResourceViewType* resourceView) const;

        //! Returns view based on the descriptor
        template <typename DescriptorType>
        auto GetResourceView(const ResourceType* resource, const DescriptorType& viewDescriptor) const -> Ptr<typename ResourceTypeHelper<ResourceType>::template ViewType<DescriptorType>>;

        //! Called by GetResourceView to insert a new view
        template <typename DescriptorType>
        auto InsertNewView(
                     const ResourceType* resource, HashValue64 hash, const DescriptorType& viewDescriptor)
                     const->Ptr<typename ResourceTypeHelper<ResourceType>::template ViewType<DescriptorType>>;

        //! Cache the ResourceViewTypes in order to avoid re-creation
        mutable AZStd::unordered_map<size_t, typename ResourceTypeHelper<ResourceType>::ResourceViewType*> m_resourceViewCache;
        //! This provides thread-safe access to the m_resourceViewCache
        mutable AZStd::mutex m_cacheMutex;
    };
}