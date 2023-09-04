/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/MultiDeviceResource.h>
#include <Atom/RHI/MultiDeviceShaderResourceGroupData.h>
#include <Atom/RHI/ShaderResourceGroup.h>

namespace AZ::RHI
{
    //! This class is a platform-independent base class for a multi-device shader resource group. It has a
    //! pointer to the multi-device resource group pool, if the user initialized the group onto a pool.
    class MultiDeviceShaderResourceGroup : public MultiDeviceResource
    {
        friend class MultiDeviceShaderResourceGroupPool;

    public:
        AZ_CLASS_ALLOCATOR(MultiDeviceShaderResourceGroup, AZ::SystemAllocator, 0);
        AZ_RTTI(MultiDeviceShaderResourceGroup, "{6C1B42AA-51A9-482F-9203-6415CA9373B7}", MultiDeviceResource);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(ShaderResourceGroup);
        MultiDeviceShaderResourceGroup() = default;
        virtual ~MultiDeviceShaderResourceGroup() override = default;

        using CompileMode = ShaderResourceGroup::CompileMode;

        //! Compiles the SRG with the provided data.
        //! When using Async compile mode, it queues a request that the parent pool compile this group (compilation is deferred).
        //! When using Sync compile mode the SRG compilation will happen immediately.
        void Compile(const MultiDeviceShaderResourceGroupData& shaderResourceGroupData, CompileMode compileMode = CompileMode::Async);

        //! Returns the shader resource group pool that this group is registered on.
        MultiDeviceShaderResourceGroupPool* GetPool();
        const MultiDeviceShaderResourceGroupPool* GetPool() const;

        //! Returns the data currently bound on the shader resource group.
        const MultiDeviceShaderResourceGroupData& GetData() const;

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
        void EnableRhiResourceTypeCompilation(const MultiDeviceShaderResourceGroupData::ResourceTypeMask resourceTypeMask);

        //! Reset the iteration counter to 0 for a resource type which will ensure that the given type will
        //! be compiled for another m_updateMaskResetLatency number of Compile calls
        void ResetResourceTypeIteration(const MultiDeviceShaderResourceGroupData::ResourceType resourceType);

        //! Return the view hash stored within m_viewHash
        HashValue64 GetViewHash(const AZ::Name& viewName);

        //! Update the view hash within m_viewHash
        void UpdateViewHash(const AZ::Name& viewName, const HashValue64 viewHash);

        //! Shuts down the resource by detaching it from its parent pool.
        void Shutdown() override final;

        //! Invalidate all views by setting off events on all device-specific ResourceInvalidateBusses
        void InvalidateViews() override final;

    private:
        MultiDeviceShaderResourceGroupData m_mdData;

        //! The binding slot cached from the layout.
        uint32_t m_bindingSlot = aznumeric_cast<uint32_t>(-1);

        //! Track hash related to views. This will help ensure we compile views in case they get invalidated and partial srg compilation
        //! is enabled
        AZStd::unordered_map<AZ::Name, HashValue64> m_viewHash;
    };
} // namespace AZ::RHI
