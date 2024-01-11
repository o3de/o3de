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
    class SingleDeviceResource;
    class SingleDeviceShaderResourceGroup;

    //! This data structure associates SingleDeviceResource invalidation events with Shader Resource Group compilation events.
    //!
    //! Shader Resource Groups (SRG's) can hold buffer and image views. These views point to resources (buffers and images)
    //! which can become invalid in several specific cases:
    //!
    //!  - The user shuts down and re-initializes a SingleDeviceBuffer / SingleDeviceImage. This effectively invalidates the platform data of all child
    //!    views and the SRG's which hold them.
    //!
    //!  - A SingleDeviceBuffer / Image pool assigns a new backing platform resource or redefines the descriptor of said resource (e.g. by
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
        using CompileGroupFunction = AZStd::function<void(SingleDeviceShaderResourceGroup&)>;

        ShaderResourceGroupInvalidateRegistry() = default;

        void SetCompileGroupFunction(CompileGroupFunction compileGroupFunction);

        void OnAttach(const SingleDeviceResource* resource, SingleDeviceShaderResourceGroup* shaderResourceGroup);

        void OnDetach(const SingleDeviceResource* resource, SingleDeviceShaderResourceGroup* shaderResourceGroup);

        bool IsEmpty() const;

    private:
        //////////////////////////////////////////////////////////////////////////
        // ResourceInvalidateBus::Handler
        ResultCode OnResourceInvalidate() override;
        //////////////////////////////////////////////////////////////////////////

        /// Attach and Detach can happen multiple times for the same SRG, if the SRG
        /// uses multiple views to the same resource (or the same view multiple times).
        using RefCountType = uint32_t;

        using Registry = AZStd::unordered_map<SingleDeviceShaderResourceGroup*, RefCountType>;
        using ResourceToRegistry = AZStd::unordered_map<const SingleDeviceResource*, Registry>;

        ResourceToRegistry m_resourceToRegistryMap;
        CompileGroupFunction m_compileGroupFunction;
    };
}
