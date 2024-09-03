/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Format.h>

namespace AZ::WebGPU
{
    const char* ToString(wgpu::BackendType type);
    const char* ToString(wgpu::DeviceLostReason reason);
    const char* ToString(wgpu::ErrorType type);
    const char* ToString(wgpu::Status status);
    wgpu::TextureFormat ConvertFormat(RHI::Format format, bool raiseAsserts = true);
    RHI::Format ConvertFormat(wgpu::TextureFormat format);
    wgpu::TextureDimension ConvertImageDimension(RHI::ImageDimension dimension);
    wgpu::Extent3D ConvertImageSize(const RHI::Size& size);
    wgpu::TextureUsage ConvertImageBinding(RHI::ImageBindFlags flags);
    wgpu::TextureAspect ConvertImageAspect(RHI::ImageAspect imageAspect);
    wgpu::TextureAspect ConvertImageAspectFlags(RHI::ImageAspectFlags flags);
}
