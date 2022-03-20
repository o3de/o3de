/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>

namespace AZ::RHI
{
    class ShaderResourceGroupData;

    // Terminology:
    // SRGB - In this context SRGB doesn't refer to the color space, but is shorthand for the class name (ShaderResourceGroupBindless)
    // Indirection value - an offset from the base of the global descriptor tables used as an index to a resource array in a shader
    // Indirection buffer - a set of indirection values
    class ShaderResourceGroupBindless
    {
    public:
        struct BindlessResourceViews
        {
            AZStd::vector<ConstPtr<ResourceView>> m_resources;
        };

        void SetViews(
            ShaderInputBufferIndex indirectResourceBufferIndex,
            const RHI::BufferView* indirectResourceBuffer,
            AZStd::span<const ImageView* const> imageViews,
            uint32_t* outIndices,
            bool viewReadOnly = true,
            uint32_t arrayIndex = 0);
        void SetViews(
            ShaderInputBufferIndex indirectResourceBufferIndex,
            const RHI::BufferView* indirectResourceBuffer,
            AZStd::span<const BufferView* const> bufferViews,
            uint32_t* outIndices,
            bool viewReadOnly = true,
            uint32_t arrayIndex = 0);

    private:
        friend class ShaderResourceGroupData;

        ShaderResourceGroupData* m_parent = nullptr;

        // The map below is used to manage ownership of buffer and image views that aren't bound directly to the shader, but implicitly
        // referenced through indirection constants. The key corresponds to the pair of (buffer input slot, index) where the indirection
        // constants reside (an array of indirection buffers is supported)
        AZStd::unordered_map<AZStd::pair<ShaderInputBufferIndex, uint32_t>, BindlessResourceViews> m_bindlessResourceViews;
    };
} // namespace AZ::RHI
