/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "RHI/Atom_RHI_DX12_precompiled.h"
#include <RHI/Conversions.h>

namespace AZ
{
    namespace DX12
    {
        namespace Platform
        {
            void FinalizeConvertBufferDescriptor(
                [[maybe_unused]] const RHI::BufferDescriptor& descriptor,
                [[maybe_unused]] D3D12_RESOURCE_DESC& resourceDesc)
            {
            }

            void FinalizeConvertImageDescriptor(
                [[maybe_unused]] const RHI::ImageDescriptor& descriptor,
                [[maybe_unused]] D3D12_RESOURCE_DESC& resourceDesc)
            {
            }

            void FinalizeConvertBufferView(
                [[maybe_unused]] const Buffer& buffer,
                [[maybe_unused]] const RHI::BufferViewDescriptor& bufferViewDescriptor,
                [[maybe_unused]] D3D12_SHADER_RESOURCE_VIEW_DESC& shaderResourceView)
            {
            }

            void FinalizeConvertBufferView(
                [[maybe_unused]] const Buffer& buffer,
                [[maybe_unused]] const RHI::BufferViewDescriptor& bufferViewDescriptor,
                [[maybe_unused]] D3D12_UNORDERED_ACCESS_VIEW_DESC& unorderedAccessView)
            {
            }

            void FinalizeConvertBufferView(
                [[maybe_unused]] const Buffer& buffer,
                [[maybe_unused]] const RHI::BufferViewDescriptor& bufferViewDescriptor,
                [[maybe_unused]] D3D12_CONSTANT_BUFFER_VIEW_DESC& constantBufferView)
            {
            }

            void FinalizeConvertImageView(
                [[maybe_unused]] const Image& image,
                [[maybe_unused]] const RHI::ImageViewDescriptor& imageViewDescriptor,
                [[maybe_unused]] D3D12_RENDER_TARGET_VIEW_DESC& renderTargetView)
            {
            }

            void FinalizeConvertImageView(
                [[maybe_unused]] const Image& image,
                [[maybe_unused]] const RHI::ImageViewDescriptor& imageViewDescriptor,
                [[maybe_unused]] D3D12_DEPTH_STENCIL_VIEW_DESC& depthStencilView)
            {
            }

            void FinalizeConvertImageView(
                [[maybe_unused]] const Image& image,
                [[maybe_unused]] const RHI::ImageViewDescriptor& imageViewDescriptor,
                [[maybe_unused]] D3D12_SHADER_RESOURCE_VIEW_DESC& shaderResourceView)
            {
            }

            void FinalizeConvertImageView(
                [[maybe_unused]] const Image& image,
                [[maybe_unused]] const RHI::ImageViewDescriptor& imageViewDescriptor,
                [[maybe_unused]] D3D12_UNORDERED_ACCESS_VIEW_DESC& unorderedAccessView)
            {
            }
        }
    }
}
