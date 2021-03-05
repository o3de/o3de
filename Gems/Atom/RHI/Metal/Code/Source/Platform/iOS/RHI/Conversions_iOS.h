/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */
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
