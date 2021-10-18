/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/ShaderResourceGroupPool.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/ImageView.h>
#include <AzCore/Debug/EventTrace.h>

namespace AZ
{
    namespace RHI
    {
        ShaderResourceGroupPool::ShaderResourceGroupPool() {}

        ShaderResourceGroupPool::~ShaderResourceGroupPool() {}

        ResultCode ShaderResourceGroupPool::Init(Device& device, const ShaderResourceGroupPoolDescriptor& descriptor)
        {
            if (Validation::IsEnabled())
            {
                if (descriptor.m_layout == nullptr)
                {
                    AZ_Error("ShaderResourceGroupPool", false, "ShaderResourceGroupPoolDescriptor::m_layout must not be null.");
                    return ResultCode::InvalidArgument;
                }
            }

            ResultCode resultCode = ResourcePool::Init(
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
                [this](ShaderResourceGroup& shaderResourceGroup)
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

        void ShaderResourceGroupPool::ShutdownInternal()
        {
            AZ_Error("ShaderResourceGroupPool", m_invalidateRegistry.IsEmpty(), "ShaderResourceGroup Registry is not Empty!");
        }

        ResultCode ShaderResourceGroupPool::InitGroup(ShaderResourceGroup& group)
        {
            ResultCode resultCode = ResourcePool::InitResource(&group, [this, &group]() { return InitGroupInternal(group); });
            if (resultCode == ResultCode::Success)
            {
                const ShaderResourceGroupLayout* layout = GetLayout();

                // Pre-initialize the data so that we can build view diffs later.
                group.m_data = ShaderResourceGroupData(layout);

                // Cache off the binding slot for one less indirection.
                group.m_bindingSlot = layout->GetBindingSlot();
            }
            return resultCode;
        }

        void ShaderResourceGroupPool::ShutdownResourceInternal(Resource& resourceBase)
        {
            ShaderResourceGroup& shaderResourceGroup = static_cast<ShaderResourceGroup&>(resourceBase);

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

            shaderResourceGroup.SetData(ShaderResourceGroupData());
        }

        void ShaderResourceGroupPool::QueueForCompile(ShaderResourceGroup& shaderResourceGroup, const ShaderResourceGroupData& groupData)
        {
            AZStd::lock_guard<AZStd::shared_mutex> lock(m_groupsToCompileMutex);

            bool isQueuedForCompile = shaderResourceGroup.IsQueuedForCompile();
            AZ_Warning(
                "ShaderResourceGroupPool", !isQueuedForCompile,
                "Attempting to compile an SRG that's already been queued for compile. Only compile an SRG once per frame.");            

            if (!isQueuedForCompile)
            {
                CalculateGroupDataDiff(shaderResourceGroup, groupData);

                shaderResourceGroup.SetData(groupData);

                QueueForCompileNoLock(shaderResourceGroup);
            }
        }

        void ShaderResourceGroupPool::QueueForCompile(ShaderResourceGroup& group)
        {
            AZStd::lock_guard<AZStd::shared_mutex> lock(m_groupsToCompileMutex);
            QueueForCompileNoLock(group);
        }

        void ShaderResourceGroupPool::QueueForCompileNoLock(ShaderResourceGroup& group)
        {
            if (!group.m_isQueuedForCompile)
            {
                group.m_isQueuedForCompile = true;
                m_groupsToCompile.emplace_back(&group);
            }
        }

        void ShaderResourceGroupPool::UnqueueForCompile(ShaderResourceGroup& shaderResourceGroup)
        {
            AZStd::lock_guard<AZStd::shared_mutex> lock(m_groupsToCompileMutex);
            if (shaderResourceGroup.m_isQueuedForCompile)
            {
                shaderResourceGroup.m_isQueuedForCompile = false;
                m_groupsToCompile.erase(AZStd::find(m_groupsToCompile.begin(), m_groupsToCompile.end(), &shaderResourceGroup));
            }
        }

        void ShaderResourceGroupPool::Compile(ShaderResourceGroup& group, const ShaderResourceGroupData& groupData)
        {
            CalculateGroupDataDiff(group, groupData);
            group.SetData(groupData);
            CompileGroupInternal(group, group.GetData());
        }

        void ShaderResourceGroupPool::CalculateGroupDataDiff(ShaderResourceGroup& shaderResourceGroup, const ShaderResourceGroupData& groupData)
        {
            // Calculate diffs for updating the resource registry.
            if (HasImageGroup() || HasBufferGroup())
            {
                /**
                 * SRG's hold references to views, and views references to resources. Resources can become invalid, either
                 * due to an explicit Shutdown() / Init() event, or an explicit call to RHI::Resource::Invalidate. In either
                 * case, the SRG will need to be re-compiled.
                 *
                 * To facilitate this, we compare the new data with the previous data and compare views. When views are attached
                 * and detached from SRG's, we store those associations in an SRG-pool local registry. The system currently takes a
                 * lock in order to build the diffs. This means compiling multiple SRG's on the same pool across several jobs is not
                 * going to be performant if the SRG's have buffers / images embedded.
                 *
                 * FUTURE CONSIDERATIONS:
                 *
                 *  - If buffers and images are initialized at allocation time instead of separately, it would only be necessary to track
                 *    resources which the platform can invalidate. This may result in a smaller set to track. There's insufficient data
                 *    to determine if this is the case right now.
                 *
                 *  - The locking could be reduced by making the registry lockless (which would be tricky, if not impossible, since it's
                 *    a map of maps), or by reducing the granularity of locks (perhaps by having multiple registries).
                 */

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
                    AZStd::array_view<ConstPtr<ImageView>> viewGroupOld = shaderResourceGroup.GetData().GetImageGroup();
                    AZStd::array_view<ConstPtr<ImageView>> viewGroupNew = groupData.GetImageGroup();
                    AZ_Assert(viewGroupOld.size() == viewGroupNew.size(), "ShaderResourceGroupData layouts do not match.");
                    for (size_t i = 0; i < viewGroupOld.size(); ++i)
                    {
                        ComputeDiffs(viewGroupOld[i].get(), viewGroupNew[i].get());
                    }
                }

                // Generate diffs for buffer views.
                if (HasBufferGroup())
                {
                    AZStd::array_view<ConstPtr<BufferView>> viewGroupOld = shaderResourceGroup.GetData().GetBufferGroup();
                    AZStd::array_view<ConstPtr<BufferView>> viewGroupNew = groupData.GetBufferGroup();
                    AZ_Assert(viewGroupOld.size() == viewGroupNew.size(), "ShaderResourceGroupData layouts do not match.");
                    for (size_t i = 0; i < viewGroupOld.size(); ++i)
                    {
                        ComputeDiffs(viewGroupOld[i].get(), viewGroupNew[i].get());
                    }
                }
            }
        }

        void ShaderResourceGroupPool::CompileGroupsBegin()
        {
            AZ_Assert(m_isCompiling == false, "Already compiling! Deadlock imminent.");
            m_groupsToCompileMutex.lock();
            m_isCompiling = true;
        }

        void ShaderResourceGroupPool::CompileGroupsEnd()
        {
            AZ_Assert(m_isCompiling, "CompileGroupsBegin() was never called.");
            m_isCompiling = false;
            m_groupsToCompile.clear();
            m_groupsToCompileMutex.unlock();
        }

        uint32_t ShaderResourceGroupPool::GetGroupsToCompileCount() const
        {
            AZ_Assert(m_isCompiling, "You must call this function within a CompileGroups{Begin, End} region!");
            return static_cast<uint32_t>(m_groupsToCompile.size());
        }

        void ShaderResourceGroupPool::CompileGroupsForInterval(Interval interval)
        {
            AZ_TRACE_METHOD_NAME("CompileGroupsForInterval");

            AZ_Assert(m_isCompiling, "You must call CompileGroupsBegin() first!");
            AZ_Assert(
                interval.m_max >= interval.m_min &&
                interval.m_max <= static_cast<uint32_t>(m_groupsToCompile.size()),
                "You must specify a valid interval for compilation");

            for (uint32_t i = interval.m_min; i < interval.m_max; ++i)
            {
                ShaderResourceGroup* group = m_groupsToCompile[i];
                CompileGroupInternal(*group, group->GetData());
                group->m_isQueuedForCompile = false;
            }
        }

        ResultCode ShaderResourceGroupPool::InitInternal(Device&, const ShaderResourceGroupPoolDescriptor&)
        {
            return ResultCode::Success;
        }

        ResultCode ShaderResourceGroupPool::InitGroupInternal(ShaderResourceGroup&)
        {
            return ResultCode::Success;
        }

        const ShaderResourceGroupPoolDescriptor& ShaderResourceGroupPool::GetDescriptor() const
        {
            return m_descriptor;
        }

        const ShaderResourceGroupLayout* ShaderResourceGroupPool::GetLayout() const
        {
            AZ_Assert(m_descriptor.m_layout, "Shader resource group layout is null");
            return m_descriptor.m_layout.get();
        }

        bool ShaderResourceGroupPool::HasConstants() const
        {
            return m_hasConstants;
        }

        bool ShaderResourceGroupPool::HasBufferGroup() const
        {
            return m_hasBufferGroup;
        }

        bool ShaderResourceGroupPool::HasImageGroup() const
        {
            return m_hasImageGroup;
        }

        bool ShaderResourceGroupPool::HasSamplerGroup() const
        {
            return m_hasSamplerGroup;
        }
    }
}
