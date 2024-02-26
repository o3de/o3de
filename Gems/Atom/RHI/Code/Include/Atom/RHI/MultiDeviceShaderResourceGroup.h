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
#include <Atom/RHI/SingleDeviceShaderResourceGroup.h>

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

        using CompileMode = SingleDeviceShaderResourceGroup::CompileMode;

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

        //! Shuts down the resource by detaching it from its parent pool.
        void Shutdown() override final;

    private:
        MultiDeviceShaderResourceGroupData m_mdData;

        //! The binding slot cached from the layout.
        uint32_t m_bindingSlot = aznumeric_cast<uint32_t>(-1);
    };
} // namespace AZ::RHI
