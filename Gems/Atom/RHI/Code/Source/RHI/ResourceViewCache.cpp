/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

 #include <Atom/RHI/ResourceViewCache.h>
 #include <Atom/RHI/ImageView.h>
 #include <Atom/RHI/BufferView.h>
 #include <Atom/RHI/Factory.h>
 #include <Atom/RHI/DeviceResource.h>
 #include <Atom/RHI/Resource.h>

 namespace AZ::RHI
 {
    Ptr<DeviceImageView> ResourceViewCache<DeviceResource>::ResourceViewTypeHelper<DeviceResource, ImageViewDescriptor>::CreateView(const DeviceResource* resource, const ImageViewDescriptor& imageViewDescriptor)
    {
        Ptr<DeviceImageView> imageViewPtr = RHI::Factory::Get().CreateImageView();
        return (imageViewPtr->Init(static_cast<const DeviceImage&>(*resource), imageViewDescriptor) == RHI::ResultCode::Success) ? imageViewPtr : nullptr;
    }

    Ptr<DeviceBufferView> ResourceViewCache<DeviceResource>::ResourceViewTypeHelper<DeviceResource, BufferViewDescriptor>::CreateView(const DeviceResource* resource, const BufferViewDescriptor& bufferViewDescriptor)
    {
        Ptr<DeviceBufferView> bufferViewPtr = RHI::Factory::Get().CreateBufferView();
        return (bufferViewPtr->Init(static_cast<const DeviceBuffer&>(*resource), bufferViewDescriptor) == RHI::ResultCode::Success) ? bufferViewPtr : nullptr;
    }

    Ptr<ImageView> ResourceViewCache<Resource>::ResourceViewTypeHelper<Resource, ImageViewDescriptor>::CreateView(const Resource* resource, const ImageViewDescriptor& imageViewDescriptor)
    {
        return aznew ImageView{static_cast<const Image*>(resource), imageViewDescriptor, resource->GetDeviceMask()};
    }

    Ptr<BufferView> ResourceViewCache<Resource>::ResourceViewTypeHelper<Resource, BufferViewDescriptor>::CreateView(const Resource* resource, const BufferViewDescriptor& bufferViewDescriptor)
    {
        return aznew BufferView{static_cast<const Buffer*>(resource), bufferViewDescriptor, resource->GetDeviceMask()};
    }

    template <typename ResourceType>
    template <typename DescriptorType>
    auto ResourceViewCache<ResourceType>::GetResourceView(const ResourceType* resource, const DescriptorType& viewDescriptor) const -> Ptr<typename ResourceViewCache<ResourceType>::ResourceTypeHelper<ResourceType>::template ViewType<DescriptorType>>
    {
        const HashValue64 hash = viewDescriptor.GetHash();
        AZStd::lock_guard<AZStd::mutex> registryLock(m_cacheMutex);
        auto it = m_resourceViewCache.find(static_cast<uint64_t>(hash));
        if (it == m_resourceViewCache.end())
        {
            return InsertNewView(resource, hash, viewDescriptor);
        }
        else
        {
            // We've found a matching ResourceViewType in the cache, but another thread may be releasing the last intrusive_ptr while
            // we are in this function, dropping the refcount to 0 (and forcing it to -1 for good measure) then deleting it.
            //
            //  There are 2 scenarios:
            //
            // m_useCount is -1. The other thread is already on the path to deleting it. We need to make a new one here
            // and replace the old one.
            //
            // m_useCount is >=0. We cannot guarantee another thread won't drop the refcount to 0 after we check the value here
            // so before we create a new intrusive_ptr, we need to use fetch_add to increment the refcount as we check the value to
            // prevent a race
            int useCount = it->second->m_useCount.fetch_add(2);
            if (useCount == -1)
            {
                // The useCount was -1 before we incremented.
                // Another thread is going to come along and delete the one we just found.
                // Go ahead and erase it and insert a new one instead
                m_resourceViewCache.erase(it);

                return InsertNewView(resource, hash, viewDescriptor);
            }
            else
            {
                // Create the new Ptr, increasing the refcount
                Ptr<typename ResourceTypeHelper<ResourceType>::template ViewType<DescriptorType>> result = static_cast<typename ResourceTypeHelper<ResourceType>::template ViewType<DescriptorType>*>(it->second);

                // Before we checked the value we artificially incremented the refcount to prevent another
                // thread from letting it go to 0 again. Get rid of that artificial increase now that we have
                // our new Ptr to hold on to the refcount
                it->second->m_useCount.fetch_sub(2);
                return result;
            }
        }       
    }

    template <typename ResourceType>
    template <typename DescriptorType>
    auto ResourceViewCache<ResourceType>::InsertNewView(const ResourceType* resource,  HashValue64 hash, const DescriptorType& viewDescriptor) const -> Ptr<typename ResourceViewCache<ResourceType>::ResourceTypeHelper<ResourceType>::template ViewType<DescriptorType>>
    {
        auto viewPtr{ResourceViewTypeHelper<ResourceType, DescriptorType>::CreateView(resource, viewDescriptor)};
        if (viewPtr)
        {
            m_resourceViewCache[static_cast<uint64_t>(hash)] = static_cast<typename ResourceTypeHelper<ResourceType>::ResourceViewType*>(viewPtr.get());
            return viewPtr;
        }
        else
        {
            return nullptr;
        }
    }

    template <typename ResourceType>
    void ResourceViewCache<ResourceType>::EraseResourceView(typename ResourceTypeHelper<ResourceType>::ResourceViewType* resourceView) const
    {
        AZStd::lock_guard<AZStd::mutex> registryLock(m_cacheMutex);
        auto itr = m_resourceViewCache.begin();
        while (itr != m_resourceViewCache.end())
        {
            if (itr->second == resourceView)
            {
                m_resourceViewCache.erase(itr->first);
                break;
            }
            itr++;
        }
    }
    
    template <typename ResourceType> 
    template <typename DescriptorType>
    bool ResourceViewCache<ResourceType>::IsInResourceCache(const DescriptorType& viewDescriptor)
    {
        const HashValue64 hash = viewDescriptor.GetHash();
        auto it = m_resourceViewCache.find(static_cast<uint64_t>(hash));
        return it != m_resourceViewCache.end();
    }

    // Explicit instantiations for DeviceResource
    template 
    Ptr<DeviceImageView> ResourceViewCache<DeviceResource>::GetResourceView(const DeviceResource* resource, const ImageViewDescriptor& imageViewDescriptor) const;
    template
    Ptr<DeviceBufferView> ResourceViewCache<DeviceResource>::GetResourceView(const DeviceResource* resource, const BufferViewDescriptor& bufferViewDescriptor) const;
    template
    Ptr<DeviceImageView> ResourceViewCache<DeviceResource>::InsertNewView(const DeviceResource* resource,  HashValue64 hash, const ImageViewDescriptor& imageViewDescriptor) const;
    template
    Ptr<DeviceBufferView> ResourceViewCache<DeviceResource>::InsertNewView(const DeviceResource* resource,  HashValue64 hash, const BufferViewDescriptor& bufferViewDescriptor) const;
    template
    void ResourceViewCache<DeviceResource>::EraseResourceView(DeviceResourceView* resourceView) const;
    template
    bool ResourceViewCache<DeviceResource>::IsInResourceCache(const ImageViewDescriptor& imageViewDescriptor);
    template
    bool ResourceViewCache<DeviceResource>::IsInResourceCache(const BufferViewDescriptor& bufferViewDescriptor);

    // Explicit instantiations for Resource
    template 
    Ptr<ImageView> ResourceViewCache<Resource>::GetResourceView(const Resource* resource, const ImageViewDescriptor& imageViewDescriptor) const;
    template
    Ptr<BufferView> ResourceViewCache<Resource>::GetResourceView(const Resource* resource, const BufferViewDescriptor& bufferViewDescriptor) const;
    template
    Ptr<ImageView> ResourceViewCache<Resource>::InsertNewView(const Resource* resource,  HashValue64 hash, const ImageViewDescriptor& imageViewDescriptor) const;
    template
    Ptr<BufferView> ResourceViewCache<Resource>::InsertNewView(const Resource* resource,  HashValue64 hash, const BufferViewDescriptor& bufferViewDescriptor) const;
    template
    void ResourceViewCache<Resource>::EraseResourceView(ResourceView* resourceView) const;
    template
    bool ResourceViewCache<Resource>::IsInResourceCache(const ImageViewDescriptor& imageViewDescriptor);
    template
    bool ResourceViewCache<Resource>::IsInResourceCache(const BufferViewDescriptor& bufferViewDescriptor);
}