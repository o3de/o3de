/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ResourceInvalidateBus.h>

namespace AZ::RHI
{
    class DeviceResource;
    class DeviceShaderResourceGroup;

    //! This data structure associates DeviceResource invalidation events with Shader Resource Group compilation events.
    //!
    //! Shader Resource Groups (SRG's) can hold buffer and image views. These views point to resources (buffers and images)
    //! which can become invalid in several specific cases:
    //!
    //!  - The user shuts down and re-initializes a DeviceBuffer / DeviceImage. This effectively invalidates the platform data of all child
    //!    views and the SRG's which hold them.
    //!
    //!  - A DeviceBuffer / Image pool assigns a new backing platform resource or redefines the descriptor of said resource (e.g. by
    //!    making certain mip levels in an image inaccessible for streaming). This can occur due to DMA memory orphaning, heap
    //!    de-fragmentation, etc.
    //!
    //! The SRG pool tracks resources as they are attached / detached from an SRG. This is done by building diff's between
    //! the old SRG data and new SRG data, and then calling OnAttach and OnDetach, respectively. Finally, resource
    //! invalidation events will result in the provided "CompileGroup" function being called for each SRG.
    //!
    //! Limitations:
    //!
    //! The registry does not hold strong references, as the cost of incrementing / decrementing atomic ref-counts would be
    //! very expensive.
    //!
    //! The registry is not thread-safe. It needs to be externally synchronized.
    class ShaderResourceGroupInvalidateRegistry
        : public ResourceInvalidateBus::MultiHandler
    {
    public:
        using CompileGroupFunction = AZStd::function<void(DeviceShaderResourceGroup&)>;

        ShaderResourceGroupInvalidateRegistry() = default;

        void SetCompileGroupFunction(CompileGroupFunction compileGroupFunction);

        void OnAttach(const DeviceResource* resource, DeviceShaderResourceGroup* shaderResourceGroup);

        void OnDetach(const DeviceResource* resource, DeviceShaderResourceGroup* shaderResourceGroup);

        bool IsEmpty() const;

    private:
        //////////////////////////////////////////////////////////////////////////
        // ResourceInvalidateBus::Handler
        ResultCode OnResourceInvalidate() override;
        //////////////////////////////////////////////////////////////////////////

        /// Attach and Detach can happen multiple times for the same SRG, if the SRG
        /// uses multiple views to the same resource (or the same view multiple times).
        using RefCountType = uint32_t;

        using Registry = AZStd::unordered_map<DeviceShaderResourceGroup*, RefCountType>;
        using ResourceToRegistry = AZStd::unordered_map<const DeviceResource*, Registry>;

        ResourceToRegistry m_resourceToRegistryMap;
        CompileGroupFunction m_compileGroupFunction;
    };
}
