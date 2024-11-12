/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI.Reflect/ClearValue.h>
#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/InputStreamLayout.h>
#include <Atom/RHI.Reflect/MemoryEnums.h>
#include <Atom/RHI.Reflect/RenderStates.h>
#include <Atom/RHI.Reflect/SamplerState.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <Atom/RHI/DeviceIndexBufferView.h>

namespace AZ::WebGPU
{
    const char* ToString(wgpu::BackendType type);
    const char* ToString(wgpu::DeviceLostReason reason);
    const char* ToString(wgpu::ErrorType type);
    const char* ToString(wgpu::Status status);
    wgpu::TextureFormat ConvertImageFormat(RHI::Format format, bool raiseAsserts = true);
    RHI::Format ConvertImageFormat(wgpu::TextureFormat format);
    wgpu::VertexFormat ConvertVertexFormat(RHI::Format format);
    wgpu::TextureDimension ConvertImageDimension(RHI::ImageDimension dimension);
    wgpu::Extent3D ConvertImageSize(const RHI::Size& size);
    wgpu::TextureUsage ConvertImageBinding(RHI::ImageBindFlags flags);
    wgpu::TextureAspect ConvertImageAspect(RHI::ImageAspect imageAspect);
    wgpu::TextureAspect ConvertImageAspectFlags(RHI::ImageAspectFlags flags);
    wgpu::PrimitiveTopology ConvertPrimitiveTopology(RHI::PrimitiveTopology topology);
    wgpu::CullMode ConvertCullMode(RHI::CullMode cullmode);
    wgpu::CompareFunction ConvertCompareFunction(RHI::ComparisonFunc func);
    wgpu::StencilOperation ConvertStencilOp(RHI::StencilOp op);
    wgpu::VertexStepMode ConvertVertexStep(RHI::StreamStepFunction step);
    wgpu::ColorWriteMask ConvertWriteMask(uint8_t writeMask);
    wgpu::BlendOperation ConvertBlendOp(RHI::BlendOp op);
    wgpu::BlendFactor ConvertBlendFactor(RHI::BlendFactor factor);
    wgpu::LoadOp ConvertLoadOp(RHI::AttachmentLoadAction action);
    wgpu::StoreOp ConvertStoreOp(RHI::AttachmentStoreAction action);
    wgpu::Color ConvertClearValue(RHI::ClearValue clearValue);
    wgpu::AddressMode ConvertAddressMode(RHI::AddressMode mode);
    wgpu::FilterMode ConvertFilterMode(RHI::FilterMode mode);
    wgpu::MipmapFilterMode ConvertMipMapFilterMode(RHI::FilterMode mode);
    wgpu::BufferUsage ConvertBufferBindFlags(RHI::BufferBindFlags flags);
    wgpu::MapMode ConvertMapMode(RHI::HostMemoryAccess access);
    wgpu::IndexFormat ConvertIndexFormat(RHI::IndexFormat format);
    wgpu::TextureViewDimension ConvertImageType(RHI::ShaderInputImageType type);
    wgpu::SamplerBindingType ConvertReductionType(RHI::ReductionType type);
    wgpu::TextureSampleType ConvertSampleType(RHI::ShaderInputImageSampleType type);
    wgpu::SamplerBindingType ConvertSamplerBindingType(RHI::ShaderInputSamplerType type);
}
