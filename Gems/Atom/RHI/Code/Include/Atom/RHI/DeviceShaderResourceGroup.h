/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceResource.h>
#include <Atom/RHI/DeviceShaderResourceGroupData.h>

namespace AZ::RHI
{
    //! This class is a platform-independent base class for a shader resource group. It has a
    //! pointer to the resource group pool, if the user initialized the group onto a pool.
    class DeviceShaderResourceGroup
        : public DeviceResource
    {
        friend class DeviceShaderResourceGroupPool;
    public:
        AZ_RTTI(DeviceShaderResourceGroup, "{91B217A5-EFEC-46C5-82DA-B4C77931BC1A}", DeviceResource);
        virtual ~DeviceShaderResourceGroup() override = default;

        //! Defines the compilation modes for an SRG
        enum class CompileMode : uint8_t
        {
            Async,  // Queues SRG compilation for later. This is the most common case.
            Sync    // Compiles the SRG immediately. To be used carefully due to performance cost.
        };

        //! Compiles the SRG with the provided data.
        //! When using Async compile mode, it queues a request that the parent pool compile this group (compilation is deferred).
        //! When using Sync compile mode the SRG compilation will happen immediately.
        void Compile(const DeviceShaderResourceGroupData& shaderResourceGroupData, CompileMode compileMode = CompileMode::Async);

        //! Returns the shader resource group pool that this group is registered on.
        DeviceShaderResourceGroupPool* GetPool();
        const DeviceShaderResourceGroupPool* GetPool() const;

        //! This implementation does not report any memory usage. Platforms may
        //! override to report more accurate usage metrics.
        void ReportMemoryUsage(MemoryStatisticsBuilder& builder) const override;

        //! Returns the data currently bound on the shader resource group.
        const DeviceShaderResourceGroupData& GetData() const;

        //! Returns the binding slot specified by the layout associated to this shader resource group.
        uint32_t GetBindingSlot() const;

        //! Returns whether the group is currently queued for compilation.
        bool IsQueuedForCompile() const;

        //! Resets the update mask after m_updateMaskResetLatency number of compiles
        void DisableCompilationForAllResourceTypes();

        //! Returns true if any of the resource type has been enabled for compilation.
        bool IsAnyResourceTypeUpdated() const;

        //! Returns true if a specific resource type has been enabled for compilation.
        bool IsResourceTypeEnabledForCompilation(uint32_t resourceTypeMask) const;

        //! Update the m_rhiUpdateMask for a given resource type which will ensure we will compile that type for the current frame
        void EnableRhiResourceTypeCompilation(const DeviceShaderResourceGroupData::ResourceTypeMask resourceTypeMask);

        //! Reset the iteration counter to 0 for a resource type which will ensure that the given type will
        //! be compiled for another m_updateMaskResetLatency number of Compile calls
        void ResetResourceTypeIteration(const DeviceShaderResourceGroupData::ResourceType resourceType);

        //! Return the view hash stored within m_viewHash
        HashValue64 GetViewHash(const AZ::Name& viewName);

        //! Update the view hash within m_viewHash
        void UpdateViewHash(const AZ::Name& viewName, const HashValue64 viewHash);
            
    protected:
        DeviceShaderResourceGroup() = default;

    private:
        void SetData(const DeviceShaderResourceGroupData& data);

        DeviceShaderResourceGroupData m_data;

        // The binding slot cached from the layout.
        uint32_t m_bindingSlot = aznumeric_cast<uint32_t>(-1);

        // Gates the Compile() function so that the SRG is only queued once.
        bool m_isQueuedForCompile = false;
            
        // Mask used to check whether to compile a specific resource type. This mask is managed on the RHI side.
        uint32_t m_rhiUpdateMask = 0;

        // Track iteration for each resource type in order to keep compiling it for m_updateMaskResetLatency number of times
        uint32_t m_resourceTypeIteration[static_cast<uint32_t>(DeviceShaderResourceGroupData::ResourceType::Count)] = { 0 };
        uint32_t m_updateMaskResetLatency = RHI::Limits::Device::FrameCountMax - 1; //we do -1 because we update after compile

        // Track hash related to views. This will help ensure we compile views in case they get invalidated and partial srg compilation is enabled
        AZStd::unordered_map<AZ::Name, HashValue64> m_viewHash;
    };
}
