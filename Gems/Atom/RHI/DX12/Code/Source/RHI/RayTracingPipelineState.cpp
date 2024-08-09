/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/RayTracingPipelineState.h>
#include <Atom/RHI.Reflect/SamplerState.h>
#include <Atom/RHI.Reflect/DX12/ShaderStageFunction.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/ShaderUtils.h>

namespace AZ
{
    namespace DX12
    {
        RHI::Ptr<RayTracingPipelineState> RayTracingPipelineState::Create()
        {
            return aznew RayTracingPipelineState;
        }

#ifdef AZ_DX12_DXR_SUPPORT
        ID3D12StateObject* RayTracingPipelineState::Get() const
        {
            return m_rayTracingPipelineState.get();
        }
#endif

        RHI::ResultCode RayTracingPipelineState::InitInternal([[maybe_unused]]RHI::Device& deviceBase, [[maybe_unused]]const RHI::DeviceRayTracingPipelineStateDescriptor* descriptor)
        {
#ifdef AZ_DX12_DXR_SUPPORT
            Device& device = static_cast<Device&>(deviceBase);

            size_t dxilLibraryCount = descriptor->GetShaderLibraries().size();
            size_t hitGroupCount = descriptor->GetHitGroups().size();
       
            // calculate the number of state sub-objects
            size_t subObjectCount =
                dxilLibraryCount +          // DXIL shader libraries
                hitGroupCount +             // hit groups
                1 +                         // payload
                1 +                         // global root signature
                1;                          // pipeline configuration
        
            // create a subobject vector with the appropriate count
            // this is pre-sized to avoid reallocation, since the API requires a contiguous array
            AZStd::vector<D3D12_STATE_SUBOBJECT> subObjects(subObjectCount);
            uint32_t currentIndex = 0;

            // Note: many of the subObjects hold pointers to Desc objects, which are kept alive
            // for the duration of this method using AZStd::vectors reserved to the appropriate size

            // add DXIL Libraries
            AZStd::vector<D3D12_DXIL_LIBRARY_DESC> libraryDescs;
            libraryDescs.reserve(dxilLibraryCount);
            AZStd::vector<ShaderByteCode> patchedShaderCache;
            for (const RHI::RayTracingShaderLibrary& shaderLibrary : descriptor->GetShaderLibraries())
            {
                const ShaderStageFunction* rayTracingFunction = azrtti_cast<const ShaderStageFunction*>(shaderLibrary.m_descriptor.m_rayTracingFunction.get());
                ShaderByteCodeView byteCode =
                    ShaderUtils::PatchShaderFunction(*rayTracingFunction, shaderLibrary.m_descriptor, patchedShaderCache);

                D3D12_DXIL_LIBRARY_DESC libraryDesc = {};
                libraryDesc.DXILLibrary = D3D12_SHADER_BYTECODE{ byteCode.data(), byteCode.size() };
                libraryDesc.NumExports = 0; // all shaders
                libraryDesc.pExports = nullptr;
                libraryDescs.push_back(libraryDesc);
        
                D3D12_STATE_SUBOBJECT librarySubObject = {};
                librarySubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
                librarySubObject.pDesc = &libraryDescs.back();
                subObjects[currentIndex++] = librarySubObject;
            }
        
            // add hit groups
            AZStd::vector<D3D12_HIT_GROUP_DESC> hitGroupDescs;
            hitGroupDescs.reserve(hitGroupCount);
            AZStd::vector<AZStd::wstring> hitGroupNameWstrings;
            hitGroupNameWstrings.reserve(hitGroupCount);
            AZStd::vector<AZStd::wstring> closestHitShaderNameWstrings;
            closestHitShaderNameWstrings.reserve(hitGroupCount);
            AZStd::vector<AZStd::wstring> anyHitShaderNameWstrings;
            anyHitShaderNameWstrings.reserve(hitGroupCount);
            AZStd::vector<AZStd::wstring> intersectionShaderNameWstrings;
            intersectionShaderNameWstrings.reserve(hitGroupCount);

            for (const RHI::RayTracingHitGroup& hitGroup : descriptor->GetHitGroups())
            {
                AZStd::wstring hitGroupNameWstring;
                AZStd::to_wstring(hitGroupNameWstring, hitGroup.m_hitGroupName.GetStringView());
                hitGroupNameWstrings.push_back(hitGroupNameWstring);

                AZStd::wstring closestHitShaderNameWstring;
                AZStd::to_wstring(closestHitShaderNameWstring, hitGroup.m_closestHitShaderName.GetStringView());
                closestHitShaderNameWstrings.push_back(closestHitShaderNameWstring);

                AZStd::wstring anyHitShaderNameWstring;
                AZStd::to_wstring(anyHitShaderNameWstring, hitGroup.m_anyHitShaderName.GetStringView());
                anyHitShaderNameWstrings.push_back(anyHitShaderNameWstring);

                AZStd::wstring intersectionShaderNameWstring;
                AZStd::to_wstring(intersectionShaderNameWstring, hitGroup.m_intersectionShaderName.GetStringView());
                intersectionShaderNameWstrings.push_back(intersectionShaderNameWstring);

                D3D12_HIT_GROUP_DESC hitGroupDesc = {};
                hitGroupDesc.Type = intersectionShaderNameWstring.empty() ? D3D12_HIT_GROUP_TYPE_TRIANGLES : D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
                hitGroupDesc.HitGroupExport = hitGroupNameWstrings.back().c_str();
                hitGroupDesc.ClosestHitShaderImport = closestHitShaderNameWstring.empty() ? nullptr : closestHitShaderNameWstrings.back().c_str();
                hitGroupDesc.AnyHitShaderImport = anyHitShaderNameWstring.empty() ? nullptr : anyHitShaderNameWstrings.back().c_str();
                hitGroupDesc.IntersectionShaderImport = intersectionShaderNameWstring.empty() ? nullptr : intersectionShaderNameWstrings.back().c_str();
                hitGroupDescs.push_back(hitGroupDesc);
        
                D3D12_STATE_SUBOBJECT hitGroupSubObject = {};
                hitGroupSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
                hitGroupSubObject.pDesc = &hitGroupDescs.back();
                subObjects[currentIndex++] = hitGroupSubObject;
            }

            // add shader payload and attribute sizes
            D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
            shaderConfig.MaxPayloadSizeInBytes = descriptor->GetConfiguration().m_maxPayloadSize;
            shaderConfig.MaxAttributeSizeInBytes = descriptor->GetConfiguration().m_maxAttributeSize;
        
            D3D12_STATE_SUBOBJECT shaderConfigSubObject = {};
            shaderConfigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
            shaderConfigSubObject.pDesc = &shaderConfig;
            subObjects[currentIndex++] = shaderConfigSubObject;

            // add global root signature
            const PipelineLayout* pipelineLayout = static_cast<const PipelineState*>(descriptor->GetPipelineState())->GetPipelineLayout();
            m_globalRootSignature = pipelineLayout->Get();
            D3D12_STATE_SUBOBJECT globalRootSignatureSubObject = {};
            globalRootSignatureSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
            globalRootSignatureSubObject.pDesc = &m_globalRootSignature;
            subObjects[currentIndex++] = globalRootSignatureSubObject;

            // note: local root signatures are not currently supported, ray tracing shaders must currently use unbounded arrays
            // [GFX TODO][ATOM-13653] AZSLc support for ray tracing local root signatures

            // add pipeline configuration
            D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
            pipelineConfig.MaxTraceRecursionDepth = descriptor->GetConfiguration().m_maxRecursionDepth;
        
            D3D12_STATE_SUBOBJECT pipelineConfigSubObject = {};
            pipelineConfigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
            pipelineConfigSubObject.pDesc = &pipelineConfig;        
            subObjects[currentIndex++] = pipelineConfigSubObject;

            // finished with objects, verify that the correct number was added to the array
            AZ_Assert(currentIndex == subObjects.size(), "mismatch in pipeline state subObject counts");

            // setup pipeline descriptor, which contains the list of subObjects
            D3D12_STATE_OBJECT_DESC pipelineDesc = {};
            pipelineDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
            pipelineDesc.NumSubobjects = currentIndex;
            pipelineDesc.pSubobjects = subObjects.data();
        
            // create the pipeline state object
            Microsoft::WRL::ComPtr<ID3D12StateObject> rayTracingPipelineStateComPtr;
            device.AssertSuccess(device.GetDevice()->CreateStateObject(&pipelineDesc, IID_GRAPHICS_PPV_ARGS(rayTracingPipelineStateComPtr.GetAddressOf())));
             m_rayTracingPipelineState = rayTracingPipelineStateComPtr.Get();
#endif  
            return RHI::ResultCode::Success;
        }

        void RayTracingPipelineState::ShutdownInternal()
        {
#ifdef AZ_DX12_DXR_SUPPORT
            static_cast<Device&>(GetDevice()).QueueForRelease(AZStd::move(m_rayTracingPipelineState));
            m_rayTracingPipelineState = nullptr;
#endif
        }
    }
}
