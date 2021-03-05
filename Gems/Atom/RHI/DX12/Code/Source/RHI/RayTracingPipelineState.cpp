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
#include "RHI/Atom_RHI_DX12_precompiled.h"
#include <RHI/RayTracingPipelineState.h>
#include <Atom/RHI.Reflect/SamplerState.h>
#include <Atom/RHI.Reflect/DX12/ShaderStageFunction.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <AzCore/Debug/EventTrace.h>

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

        void RayTracingPipelineState::CreateRootSignature(RHI::Device& deviceBase,
                                                          [[maybe_unused]]const RHI::RayTracingRootSignature& rayTracingRootSignature,
                                                          [[maybe_unused]]bool isLocalRootSignature,
                                                          [[maybe_unused]]Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSignatureComPtr)
        {
            Device& device = static_cast<Device&>(deviceBase);

#ifdef AZ_DX12_DXR_SUPPORT
            size_t numRootParameters =
                (rayTracingRootSignature.m_cbvParam.m_addedToRootSignature ? 1 : 0) +
                (rayTracingRootSignature.m_descriptorTableParam.m_addedToRootSignature ? 1 : 0);

            AZStd::vector<D3D12_ROOT_PARAMETER> rootParameters(numRootParameters);
            AZStd::vector<D3D12_DESCRIPTOR_RANGE> descriptorRanges(rayTracingRootSignature.m_descriptorTableParam.m_ranges.size());
            uint32_t rootParameterIndex = 0;
            uint32_t descriptorRangeIndex = 0;

            // build the cbv parameter
            if (rayTracingRootSignature.m_cbvParam.m_addedToRootSignature)
            {
                D3D12_ROOT_PARAMETER& cbvParam = rootParameters[rootParameterIndex++];
                cbvParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                cbvParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
                cbvParam.Descriptor.ShaderRegister = rayTracingRootSignature.m_cbvParam.m_shaderRegister;
                cbvParam.Descriptor.RegisterSpace = rayTracingRootSignature.m_cbvParam.m_registerSpace;
            }

            // build the descriptor table parameter
            if (rayTracingRootSignature.m_descriptorTableParam.m_addedToRootSignature)
            {
                D3D12_ROOT_PARAMETER& descriptorTableParam = rootParameters[rootParameterIndex++];
                descriptorTableParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

                // add the ranges
                for (const auto& range : rayTracingRootSignature.m_descriptorTableParam.m_ranges)
                {
                    D3D12_DESCRIPTOR_RANGE& descriptorRange = descriptorRanges[descriptorRangeIndex++];
                    descriptorRange.NumDescriptors = 1;
                    descriptorRange.BaseShaderRegister = range.m_shaderRegister;
                    descriptorRange.RegisterSpace = range.m_registerSpace;
                    descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

                    switch (range.m_type)
                    {
                    case RHI::RayTracingRootSignatureDescriptorTableParam::Range::Type::Constant:
                        descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                        break;
                    case RHI::RayTracingRootSignatureDescriptorTableParam::Range::Type::Read:
                        descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                        break;
                    case RHI::RayTracingRootSignatureDescriptorTableParam::Range::Type::ReadWrite:
                        descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                        break;
                    default:
                        AZ_Assert(false, "Unknown descriptor range type");
                        break;
                    };
                }

                descriptorTableParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                descriptorTableParam.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(descriptorRanges.size());
                descriptorTableParam.DescriptorTable.pDescriptorRanges = descriptorRanges.data();
            }

            // build static samplers
            uint32_t staticSamplerIndex = 0;
            AZStd::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers(rayTracingRootSignature.m_staticSamplers.size());
            for (const auto& staticSampler : rayTracingRootSignature.m_staticSamplers)
            {
                D3D12_STATIC_SAMPLER_DESC& samplerDesc = staticSamplers[staticSamplerIndex++];
                
                samplerDesc.AddressU = ConvertAddressMode(staticSampler.m_addressMode);
                samplerDesc.AddressV = ConvertAddressMode(staticSampler.m_addressMode);
                samplerDesc.AddressW = ConvertAddressMode(staticSampler.m_addressMode);
                samplerDesc.BorderColor = ConvertBorderColor(RHI::BorderColor::TransparentBlack);
                samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
                D3D12_FILTER_TYPE min = ConvertFilterMode(staticSampler.m_filterMode);
                D3D12_FILTER_TYPE mag = ConvertFilterMode(staticSampler.m_filterMode);
                D3D12_FILTER_TYPE mip = ConvertFilterMode(staticSampler.m_filterMode);
                samplerDesc.Filter = D3D12_ENCODE_BASIC_FILTER(min, mag, mip, D3D12_FILTER_REDUCTION_TYPE_STANDARD);
                samplerDesc.MaxAnisotropy = uint8_t(0);
                samplerDesc.MaxLOD = uint8_t(15);
                samplerDesc.MinLOD = uint8_t(0);
                samplerDesc.MipLODBias = 0.0f;
                samplerDesc.ShaderRegister = staticSampler.m_shaderRegister;
                samplerDesc.RegisterSpace = staticSampler.m_registerSpace;
                samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            }

            D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
            rootSignatureDesc.NumParameters = static_cast<UINT>(rootParameters.size());
            rootSignatureDesc.pParameters = rootParameters.data();
            rootSignatureDesc.Flags = isLocalRootSignature ? D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE : D3D12_ROOT_SIGNATURE_FLAG_NONE;
            rootSignatureDesc.NumStaticSamplers = static_cast<UINT>(staticSamplers.size());
            rootSignatureDesc.pStaticSamplers = staticSamplers.data();

            ID3DBlob* rootSignatureBlob = nullptr;
            ID3DBlob* errorBlob = nullptr;
            HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &errorBlob);
            if (FAILED(hr))
            {
                const char* errorString = reinterpret_cast<const char*>(errorBlob->GetBufferPointer());
                AZ_Assert(false, "D3D12SerializeRootSignature failed: [%s]", errorString);
                errorBlob->Release();
            }

            hr = device.GetDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_GRAPHICS_PPV_ARGS(rootSignatureComPtr.GetAddressOf()));
            AZ_Assert(SUCCEEDED(hr), "CreateRootSignature failed");
            rootSignatureBlob->Release();
#endif
        }

        RHI::ResultCode RayTracingPipelineState::InitInternal(RHI::Device& deviceBase, [[maybe_unused]]const RHI::RayTracingPipelineStateDescriptor* descriptor)
        {
            Device& device = static_cast<Device&>(deviceBase);

#ifdef AZ_DX12_DXR_SUPPORT
            size_t dxilLibraryCount = descriptor->GetShaderLibraries().size();
            size_t hitGroupCount = descriptor->GetHitGroups().size();
            size_t localRootSignatureCount = descriptor->GetLocalRootSignatures().size();
       
            // calculate the number of state sub-objects
            size_t subObjectCount =
                dxilLibraryCount +          // DXIL shader libraries
                hitGroupCount +             // hit groups
                localRootSignatureCount +   // local root signatures
                localRootSignatureCount +   // local root signature shader associations
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
            for (const RHI::RayTracingShaderLibrary& shaderLibrary : descriptor->GetShaderLibraries())
            {
                const ShaderStageFunction* rayTracingFunction = azrtti_cast<const ShaderStageFunction*>(shaderLibrary.m_descriptor.m_rayTracingFunction.get());
        
                D3D12_DXIL_LIBRARY_DESC libraryDesc = {};
                libraryDesc.DXILLibrary = D3D12_SHADER_BYTECODE{ rayTracingFunction->GetByteCode().data(), rayTracingFunction->GetByteCode().size() };
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

            for (const RHI::RayTracingHitGroup& hitGroup : descriptor->GetHitGroups())
            {
                AZStd::wstring hitGroupNameWstring;
                AZStd::to_wstring(hitGroupNameWstring, hitGroup.m_hitGroupName.GetStringView().data(), hitGroup.m_hitGroupName.GetStringView().size());
                hitGroupNameWstrings.push_back(hitGroupNameWstring);

                AZStd::wstring closestHitShaderNameWstring;
                AZStd::to_wstring(closestHitShaderNameWstring, hitGroup.m_closestHitShaderName.GetStringView().data(), hitGroup.m_closestHitShaderName.GetStringView().size());
                closestHitShaderNameWstrings.push_back(closestHitShaderNameWstring);

                AZStd::wstring anyHitShaderNameWstring;
                AZStd::to_wstring(anyHitShaderNameWstring, hitGroup.m_anyHitShaderName.GetStringView().data(), hitGroup.m_anyHitShaderName.GetStringView().size());
                anyHitShaderNameWstrings.push_back(anyHitShaderNameWstring);

                D3D12_HIT_GROUP_DESC hitGroupDesc = {};
                hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
                hitGroupDesc.HitGroupExport = hitGroupNameWstrings.back().c_str();
                hitGroupDesc.ClosestHitShaderImport = closestHitShaderNameWstring.empty() ? nullptr : closestHitShaderNameWstrings.back().c_str();
                hitGroupDesc.AnyHitShaderImport = anyHitShaderNameWstring.empty() ? nullptr : anyHitShaderNameWstrings.back().c_str();
                hitGroupDesc.IntersectionShaderImport = nullptr; // only triangle geometry is supported at this time
                hitGroupDescs.push_back(hitGroupDesc);
        
                D3D12_STATE_SUBOBJECT hitGroupSubObject = {};
                hitGroupSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
                hitGroupSubObject.pDesc = &hitGroupDescs.back();
                subObjects[currentIndex++] = hitGroupSubObject;
            }

            // each local root signature is associated with one or more shaders using an association subObject
            AZStd::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> localRootSignatureAssociations;
            localRootSignatureAssociations.reserve(localRootSignatureCount);

            // each association for a local root signature points to a list of shader names
            // count the total number of shader associations across all local root signatures
            size_t localRootSignatureShaderAssociationsCount = 0;
            for (auto& localRootSignature : descriptor->GetLocalRootSignatures())
            {
                localRootSignatureShaderAssociationsCount += localRootSignature.m_shaderAssociations.size();
            }

            // pre-allocate a vector to hold the names, and an index to the start of each shader name list for each local root signature
            AZStd::vector<LPCWSTR> localRootSignatureShaderNames;
            localRootSignatureShaderNames.reserve(localRootSignatureShaderAssociationsCount);
            AZStd::vector<AZStd::wstring> localRootSignatureShaderNameWstrings;
            localRootSignatureShaderNameWstrings.reserve(localRootSignatureShaderAssociationsCount);
            uint32_t localRootSignatureShaderNamesIndex = 0;

            // add local root signatures
            m_localRootSignatures.reserve(localRootSignatureCount);
            for (const RHI::RayTracingLocalRootSignature& localRootSignature : descriptor->GetLocalRootSignatures())
            {
                // create the root signature
                // [GFX TODO][ATOM-5570] Use the shader PipelineLayout to set DXR root signatures
                Microsoft::WRL::ComPtr<ID3D12RootSignature> localRootSignatureComPtr;
                CreateRootSignature(deviceBase, localRootSignature, true, localRootSignatureComPtr);
                m_localRootSignatures.push_back(localRootSignatureComPtr.Get());

                // add the root signature to the pipeline state
                D3D12_STATE_SUBOBJECT localRootSignatureSubObject;
                localRootSignatureSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
                localRootSignatureSubObject.pDesc = &m_localRootSignatures.back();
                subObjects[currentIndex++] = localRootSignatureSubObject;

                // build a list of the shader names for this local root signature
                for (auto& shaderName : localRootSignature.m_shaderAssociations)
                {
                    AZStd::wstring shaderNameWstring;
                    AZStd::to_wstring(shaderNameWstring, shaderName.GetStringView().data(), shaderName.GetStringView().size());
                    localRootSignatureShaderNameWstrings.push_back(shaderNameWstring);

                    localRootSignatureShaderNames.push_back(localRootSignatureShaderNameWstrings.back().c_str());
                }

                // add the shader association to the pipeline state
                D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderAssociation = {};
                shaderAssociation.NumExports = (UINT)localRootSignature.m_shaderAssociations.size();
                shaderAssociation.pExports = &localRootSignatureShaderNames[localRootSignatureShaderNamesIndex];
                shaderAssociation.pSubobjectToAssociate = &subObjects[(currentIndex - 1)];    // points to the local root signature subObject
                localRootSignatureAssociations.push_back(shaderAssociation);

                D3D12_STATE_SUBOBJECT shaderAssociationSubObject = {};
                shaderAssociationSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
                shaderAssociationSubObject.pDesc = &localRootSignatureAssociations.back();
                subObjects[currentIndex++] = shaderAssociationSubObject;

                localRootSignatureShaderNamesIndex += static_cast<uint32_t>(localRootSignature.m_shaderAssociations.size());
            }

            // add shader payload and attribute sizes
            D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
            shaderConfig.MaxPayloadSizeInBytes = descriptor->GetConfiguration().m_maxPayloadSize;
            shaderConfig.MaxAttributeSizeInBytes = descriptor->GetConfiguration().m_maxAttributeSize;
        
            D3D12_STATE_SUBOBJECT shaderConfigSubObject = {};
            shaderConfigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
            shaderConfigSubObject.pDesc = &shaderConfig;
            subObjects[currentIndex++] = shaderConfigSubObject;
        
            // create the global root signature
            // [GFX TODO][ATOM-5570] Use the shader PipelineLayout to set DXR root signatures
            Microsoft::WRL::ComPtr<ID3D12RootSignature> globalRootSignatureComPtr;
            CreateRootSignature(deviceBase, descriptor->GetGlobalRootSignature(), false, globalRootSignatureComPtr);
            m_globalRootSignature = globalRootSignatureComPtr.Get();

            // add global root signature
            D3D12_STATE_SUBOBJECT globalRootSignatureSubObject = {};
            globalRootSignatureSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
            globalRootSignatureSubObject.pDesc = &m_globalRootSignature;
            subObjects[currentIndex++] = globalRootSignatureSubObject;
        
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
            HRESULT hr = device.GetDevice()->CreateStateObject(&pipelineDesc, IID_GRAPHICS_PPV_ARGS(rayTracingPipelineStateComPtr.GetAddressOf()));
            AZ_Assert(SUCCEEDED(hr), "Failed to create ray tracing pipeline state");
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
