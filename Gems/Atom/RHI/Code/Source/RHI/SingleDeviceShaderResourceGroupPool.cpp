/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/SingleDeviceShaderResourceGroupPool.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/ImageView.h>
#include <AzCore/Console/IConsole.h>

namespace AZ::RHI
{
    AZ_CVAR(bool, r_DisablePartialSrgCompilation, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Enable this cvar to disable Partial SRG compilation");

    SingleDeviceShaderResourceGroupPool::SingleDeviceShaderResourceGroupPool() {}

    SingleDeviceShaderResourceGroupPool::~SingleDeviceShaderResourceGroupPool() {}

    ResultCode SingleDeviceShaderResourceGroupPool::Init(Device& device, const ShaderResourceGroupPoolDescriptor& descriptor)
    {
        if (Validation::IsEnabled())
        {
            if (descriptor.m_layout == nullptr)
            {
                AZ_Error("SingleDeviceShaderResourceGroupPool", false, "ShaderResourceGroupPoolDescriptor::m_layout must not be null.");
                return ResultCode::InvalidArgument;
            }
        }

        ResultCode resultCode = SingleDeviceResourcePool::Init(
            device, descriptor,
            [this, &device, &descriptor]()
        {
            return InitInternal(device, descriptor);
        });

        if (resultCode != ResultCode::Success)
        {
            return resultCode;
        }

        m_invalidateRegistry.SetCompileGroupFunction(
            [this](SingleDeviceShaderResourceGroup& shaderResourceGroup)
        {
            QueueForCompile(shaderResourceGroup);
        });

        m_descriptor = descriptor;
        m_hasBufferGroup = m_descriptor.m_layout->GetGroupSizeForBuffers() > 0;
        m_hasImageGroup = m_descriptor.m_layout->GetGroupSizeForImages() > 0;
        m_hasSamplerGroup = m_descriptor.m_layout->GetGroupSizeForSamplers() > 0;
        m_hasConstants = m_descriptor.m_layout->GetConstantDataSize() > 0;

        return ResultCode::Success;
    }

    void SingleDeviceShaderResourceGroupPool::ShutdownInternal()
    {
        AZ_Error("SingleDeviceShaderResourceGroupPool", m_invalidateRegistry.IsEmpty(), "SingleDeviceShaderResourceGroup Registry is not Empty!");
    }

    ResultCode SingleDeviceShaderResourceGroupPool::InitGroup(SingleDeviceShaderResourceGroup& group)
    {
        ResultCode resultCode = SingleDeviceResourcePool::InitResource(&group, [this, &group]() { return InitGroupInternal(group); });
        if (resultCode == ResultCode::Success)
        {
            const ShaderResourceGroupLayout* layout = GetLayout();

            // Pre-initialize the data so that we can build view diffs later.
            group.m_data = SingleDeviceShaderResourceGroupData(layout);

            // Cache off the binding slot for one less indirection.
            group.m_bindingSlot = layout->GetBindingSlot();
        }
        return resultCode;
    }

    void SingleDeviceShaderResourceGroupPool::ShutdownResourceInternal(SingleDeviceResource& resourceBase)
    {
        SingleDeviceShaderResourceGroup& shaderResourceGroup = static_cast<SingleDeviceShaderResourceGroup&>(resourceBase);

        UnqueueForCompile(shaderResourceGroup);

        // Cease tracking references to buffer / image resources when the SRG shuts down.
        if (HasImageGroup() || HasBufferGroup())
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_invalidateRegistryMutex);

            for (const ConstPtr<ImageView>& imageView : shaderResourceGroup.GetData().GetImageGroup())
            {
                if (imageView)
                {
                    m_invalidateRegistry.OnDetach(&imageView->GetResource(), &shaderResourceGroup);
                }
            }

            for (const ConstPtr<BufferView>& bufferView : shaderResourceGroup.GetData().GetBufferGroup())
            {
                if (bufferView)
                {
                    m_invalidateRegistry.OnDetach(&bufferView->GetResource(), &shaderResourceGroup);
                }
            }
        }

        shaderResourceGroup.SetData(SingleDeviceShaderResourceGroupData());
    }

    void SingleDeviceShaderResourceGroupPool::QueueForCompile(SingleDeviceShaderResourceGroup& shaderResourceGroup, const SingleDeviceShaderResourceGroupData& groupData)
    {
        AZStd::lock_guard<AZStd::shared_mutex> lock(m_groupsToCompileMutex);

        bool isQueuedForCompile = shaderResourceGroup.IsQueuedForCompile();
        AZ_Warning(
            "SingleDeviceShaderResourceGroupPool", !isQueuedForCompile,
            "Attempting to compile SRG '%s' that's already been queued for compile. Only compile an SRG once per frame.",
            shaderResourceGroup.GetName().GetCStr());

        if (!isQueuedForCompile)
        {
            CalculateGroupDataDiff(shaderResourceGroup, groupData);

            shaderResourceGroup.SetData(groupData);

            QueueForCompileNoLock(shaderResourceGroup);
        }
    }

    void SingleDeviceShaderResourceGroupPool::QueueForCompile(SingleDeviceShaderResourceGroup& group)
    {
        AZStd::lock_guard<AZStd::shared_mutex> lock(m_groupsToCompileMutex);
        QueueForCompileNoLock(group);
    }

    void SingleDeviceShaderResourceGroupPool::QueueForCompileNoLock(SingleDeviceShaderResourceGroup& group)
    {
        if (!group.m_isQueuedForCompile)
        {
            group.m_isQueuedForCompile = true;
            m_groupsToCompile.emplace_back(&group);
        }
    }

    void SingleDeviceShaderResourceGroupPool::UnqueueForCompile(SingleDeviceShaderResourceGroup& shaderResourceGroup)
    {
        AZStd::lock_guard<AZStd::shared_mutex> lock(m_groupsToCompileMutex);
        if (shaderResourceGroup.m_isQueuedForCompile)
        {
            shaderResourceGroup.m_isQueuedForCompile = false;
            m_groupsToCompile.erase(AZStd::find(m_groupsToCompile.begin(), m_groupsToCompile.end(), &shaderResourceGroup));
        }
    }

    void SingleDeviceShaderResourceGroupPool::Compile(SingleDeviceShaderResourceGroup& group, const SingleDeviceShaderResourceGroupData& groupData)
    {
        CalculateGroupDataDiff(group, groupData);
        group.SetData(groupData);
        CompileGroup(group, group.GetData());
    }

    void SingleDeviceShaderResourceGroupPool::CalculateGroupDataDiff(SingleDeviceShaderResourceGroup& shaderResourceGroup, const SingleDeviceShaderResourceGroupData& groupData)
    {
        // Calculate diffs for updating the resource registry.
        if (HasImageGroup() || HasBufferGroup())
        {
            // SRG's hold references to views, and views references to resources. Resources can become invalid, either
            // due to an explicit Shutdown() / Init() event, or an explicit call to RHI::SingleDeviceResource::Invalidate. In either
            // case, the SRG will need to be re-compiled.
            //
            // To facilitate this, we compare the new data with the previous data and compare views. When views are attached
            // and detached from SRG's, we store those associations in an SRG-pool local registry. The system currently takes a
            // lock in order to build the diffs. This means compiling multiple SRG's on the same pool across several jobs is not
            // going to be performant if the SRG's have buffers / images embedded.
            //
            // FUTURE CONSIDERATIONS:
            //
            //  - If buffers and images are initialized at allocation time instead of separately, it would only be necessary to track
            //    resources which the platform can invalidate. This may result in a smaller set to track. There's insufficient data
            //    to determine if this is the case right now.
            //
            //  - The locking could be reduced by making the registry lockless (which would be tricky, if not impossible, since it's
            //    a map of maps), or by reducing the granularity of locks (perhaps by having multiple registries).
            const auto ComputeDiffs = [this, &shaderResourceGroup](const ResourceView* resourceViewOld, const ResourceView* resourceViewNew)
            {
                if (resourceViewOld != resourceViewNew)
                {
                    if (resourceViewNew)
                    {
                        m_invalidateRegistry.OnAttach(&resourceViewNew->GetResource(), &shaderResourceGroup);
                    }

                    if (resourceViewOld)
                    {
                        m_invalidateRegistry.OnDetach(&resourceViewOld->GetResource(), &shaderResourceGroup);
                    }
                }
            };

            AZStd::lock_guard<AZStd::mutex> registryLock(m_invalidateRegistryMutex);

            // Generate diffs for image views.
            if (HasImageGroup())
            {
                AZStd::span<const ConstPtr<ImageView>> viewGroupOld = shaderResourceGroup.GetData().GetImageGroup();
                AZStd::span<const ConstPtr<ImageView>> viewGroupNew = groupData.GetImageGroup();
                AZ_Assert(viewGroupOld.size() == viewGroupNew.size(), "SingleDeviceShaderResourceGroupData layouts do not match.");
                for (size_t i = 0; i < viewGroupOld.size(); ++i)
                {
                    ComputeDiffs(viewGroupOld[i].get(), viewGroupNew[i].get());
                }
            }

            // Generate diffs for buffer views.
            if (HasBufferGroup())
            {
                AZStd::span<const ConstPtr<BufferView>> viewGroupOld = shaderResourceGroup.GetData().GetBufferGroup();
                AZStd::span<const ConstPtr<BufferView>> viewGroupNew = groupData.GetBufferGroup();
                AZ_Assert(viewGroupOld.size() == viewGroupNew.size(), "SingleDeviceShaderResourceGroupData layouts do not match.");
                for (size_t i = 0; i < viewGroupOld.size(); ++i)
                {
                    ComputeDiffs(viewGroupOld[i].get(), viewGroupNew[i].get());
                }
            }
        }
    }

    void SingleDeviceShaderResourceGroupPool::CompileGroupsBegin()
    {
        AZ_Assert(m_isCompiling == false, "Already compiling! Deadlock imminent.");
        m_groupsToCompileMutex.lock();
        m_isCompiling = true;
    }

    void SingleDeviceShaderResourceGroupPool::CompileGroupsEnd()
    {
        AZ_Assert(m_isCompiling, "CompileGroupsBegin() was never called.");
        m_isCompiling = false;
        m_groupsToCompile.clear();
        m_groupsToCompileMutex.unlock();
    }

    uint32_t SingleDeviceShaderResourceGroupPool::GetGroupsToCompileCount() const
    {
        AZ_Assert(m_isCompiling, "You must call this function within a CompileGroups{Begin, End} region!");
        return static_cast<uint32_t>(m_groupsToCompile.size());
    }

    template<typename T>
    HashValue64 SingleDeviceShaderResourceGroupPool::GetViewHash(AZStd::span<const RHI::ConstPtr<T>> views)
    {
        HashValue64 viewHash = HashValue64{ 0 };
        for (size_t i = 0; i < views.size(); ++i)
        {
            if (views[i])
            {
                viewHash = TypeHash64(views[i]->GetHash(), viewHash);
            }
        }
        return viewHash;
    }

    template<typename T>
    void SingleDeviceShaderResourceGroupPool::UpdateMaskBasedOnViewHash(
        SingleDeviceShaderResourceGroup& shaderResourceGroup, Name entryName, AZStd::span<const RHI::ConstPtr<T>> views,
        SingleDeviceShaderResourceGroupData::ResourceType resourceType)
    {
        //Get the view hash and check if it was updated in which case we need to compile those views. 
        HashValue64 viewHash = GetViewHash<T>(views);
        if (shaderResourceGroup.GetViewHash(entryName) != viewHash)
        {
            shaderResourceGroup.EnableRhiResourceTypeCompilation(static_cast<SingleDeviceShaderResourceGroupData::ResourceTypeMask>(AZ_BIT(static_cast<uint32_t>(resourceType))));
            shaderResourceGroup.ResetResourceTypeIteration(resourceType);
            shaderResourceGroup.UpdateViewHash(entryName, viewHash);
        }
    }

    void SingleDeviceShaderResourceGroupPool::ResetUpdateMaskForModifiedViews(
        SingleDeviceShaderResourceGroup& shaderResourceGroup, const SingleDeviceShaderResourceGroupData& shaderResourceGroupData)
    {
        const RHI::ShaderResourceGroupLayout& groupLayout = *shaderResourceGroupData.GetLayout();
        uint32_t shaderInputIndex = 0;
        //Check image views
        for (const RHI::ShaderInputImageDescriptor& shaderInputImage : groupLayout.GetShaderInputListForImages())
        {
            const RHI::ShaderInputImageIndex imageInputIndex(shaderInputIndex);
            UpdateMaskBasedOnViewHash<RHI::ImageView>(
                shaderResourceGroup, shaderInputImage.m_name, shaderResourceGroupData.GetImageViewArray(imageInputIndex),
                SingleDeviceShaderResourceGroupData::ResourceType::ImageView);
            ++shaderInputIndex;
        }

        shaderInputIndex = 0;
        //Check buffer views
        for (const RHI::ShaderInputBufferDescriptor& shaderInputBuffer : groupLayout.GetShaderInputListForBuffers())
        {
            const RHI::ShaderInputBufferIndex bufferInputIndex(shaderInputIndex);
            UpdateMaskBasedOnViewHash<RHI::BufferView>(
                shaderResourceGroup, shaderInputBuffer.m_name, shaderResourceGroupData.GetBufferViewArray(bufferInputIndex),
                SingleDeviceShaderResourceGroupData::ResourceType::BufferView);
            ++shaderInputIndex;
        }

        shaderInputIndex = 0;
        //Check unbounded image views
        for (const RHI::ShaderInputImageUnboundedArrayDescriptor& shaderInputImageUnboundedArray : groupLayout.GetShaderInputListForImageUnboundedArrays())
        {
            const RHI::ShaderInputImageUnboundedArrayIndex imageUnboundedArrayInputIndex(shaderInputIndex);
            UpdateMaskBasedOnViewHash<RHI::ImageView>(
                shaderResourceGroup, shaderInputImageUnboundedArray.m_name,
                shaderResourceGroupData.GetImageViewUnboundedArray(imageUnboundedArrayInputIndex),
                SingleDeviceShaderResourceGroupData::ResourceType::ImageViewUnboundedArray);
            ++shaderInputIndex;
        }

        shaderInputIndex = 0;
        //Check unbounded buffer views
        for (const RHI::ShaderInputBufferUnboundedArrayDescriptor& shaderInputBufferUnboundedArray : groupLayout.GetShaderInputListForBufferUnboundedArrays())
        {
            const RHI::ShaderInputBufferUnboundedArrayIndex bufferUnboundedArrayInputIndex(shaderInputIndex);
            UpdateMaskBasedOnViewHash<RHI::BufferView>(
                shaderResourceGroup, shaderInputBufferUnboundedArray.m_name,
                shaderResourceGroupData.GetBufferViewUnboundedArray(bufferUnboundedArrayInputIndex),
                SingleDeviceShaderResourceGroupData::ResourceType::BufferViewUnboundedArray);
            ++shaderInputIndex;
        }
    }

    ResultCode SingleDeviceShaderResourceGroupPool::CompileGroup(SingleDeviceShaderResourceGroup& shaderResourceGroup,
                                                        const SingleDeviceShaderResourceGroupData& shaderResourceGroupData)
    {
        if (r_DisablePartialSrgCompilation)
        {
            //Reset m_rhiUpdateMask for all resource types which will disable partial SRG compilation
            for (uint32_t i = 0; i < static_cast<uint32_t>(SingleDeviceShaderResourceGroupData::ResourceType::Count); i++)
            {
                shaderResourceGroup.EnableRhiResourceTypeCompilation(static_cast<SingleDeviceShaderResourceGroupData::ResourceTypeMask>(AZ_BIT(i)));
            }
        }

        // Modify m_rhiUpdateMask in case a view was modified. This can happen if a view is invalidated
        ResetUpdateMaskForModifiedViews(shaderResourceGroup, shaderResourceGroupData);   
            
        // Check if any part of the Srg was updated before trying to compile it
        if (shaderResourceGroup.IsAnyResourceTypeUpdated())
        {
            ResultCode resultCode = CompileGroupInternal(shaderResourceGroup, shaderResourceGroupData);
                
            //Reset update mask if the latency check has been fulfilled
            shaderResourceGroup.DisableCompilationForAllResourceTypes();
            return resultCode;
        }
        return ResultCode::Success;
    }
    
    void SingleDeviceShaderResourceGroupPool::CompileGroupsForInterval(Interval interval)
    {
        AZ_Assert(m_isCompiling, "You must call CompileGroupsBegin() first!");
        AZ_Assert(
            interval.m_max >= interval.m_min &&
            interval.m_max <= static_cast<uint32_t>(m_groupsToCompile.size()),
            "You must specify a valid interval for compilation");

        for (uint32_t i = interval.m_min; i < interval.m_max; ++i)
        {
            SingleDeviceShaderResourceGroup* group = m_groupsToCompile[i];
            RHI_PROFILE_SCOPE_VERBOSE("CompileGroupsForInterval %s", group->GetName().GetCStr());

            CompileGroup(*group, group->GetData());
            group->m_isQueuedForCompile = false;
        }
    }

    ResultCode SingleDeviceShaderResourceGroupPool::InitInternal(Device&, const ShaderResourceGroupPoolDescriptor&)
    {
        return ResultCode::Success;
    }

    ResultCode SingleDeviceShaderResourceGroupPool::InitGroupInternal(SingleDeviceShaderResourceGroup&)
    {
        return ResultCode::Success;
    }

    const ShaderResourceGroupPoolDescriptor& SingleDeviceShaderResourceGroupPool::GetDescriptor() const
    {
        return m_descriptor;
    }

    const ShaderResourceGroupLayout* SingleDeviceShaderResourceGroupPool::GetLayout() const
    {
        AZ_Assert(m_descriptor.m_layout, "Shader resource group layout is null");
        return m_descriptor.m_layout.get();
    }

    bool SingleDeviceShaderResourceGroupPool::HasConstants() const
    {
        return m_hasConstants;
    }

    bool SingleDeviceShaderResourceGroupPool::HasBufferGroup() const
    {
        return m_hasBufferGroup;
    }

    bool SingleDeviceShaderResourceGroupPool::HasImageGroup() const
    {
        return m_hasImageGroup;
    }

    bool SingleDeviceShaderResourceGroupPool::HasSamplerGroup() const
    {
        return m_hasSamplerGroup;
    }
}
