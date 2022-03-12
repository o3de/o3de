/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZ
{
    namespace Metal
    {
        namespace Platform
        {
            MTLPixelFormat ConvertPixelFormat(RHI::Format format);
            MTLBlitOption GetBlitOption(RHI::Format format);
            MTLSamplerAddressMode ConvertAddressMode(RHI::AddressMode addressMode);
            MTLResourceOptions CovertStorageMode(MTLStorageMode storageMode);
            MTLStorageMode GetCPUGPUMemoryMode();
            bool IsDepthStencilMerged(MTLPixelFormat mtlFormat);
            bool IsFilteringSupported(id<MTLDevice> mtlDevice, RHI::Format format);
            bool IsWriteSupported(id<MTLDevice> mtlDevice, RHI::Format format);
            bool IsColorRenderTargetSupported(id<MTLDevice> mtlDevice, RHI::Format format);
            bool IsBlendingSupported(id<MTLDevice> mtlDevice, RHI::Format format);
            bool IsMSAASupported(id<MTLDevice> mtlDevice, RHI::Format format);
            bool IsResolveTargetSupported(id<MTLDevice> mtlDevice, RHI::Format format);
            bool IsDepthStencilSupported(id<MTLDevice> mtlDevice, RHI::Format format);
        }
    }
}
