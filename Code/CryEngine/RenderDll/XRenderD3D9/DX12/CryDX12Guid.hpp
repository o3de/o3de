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
// Original file Copyright Crytek GMBH or its affiliates, used under license.
#pragma once

#if defined(AZ_RESTRICTED_PLATFORM)
    #undef AZ_RESTRICTED_SECTION
    #define CRYDX12GUID_HPP_SECTION_1 1
#endif

#define DX12_DEFINE_TYPE_GUID(_CLASS, _TYPE, X) \
  _CLASS __declspec(uuid(X)) _TYPE;

DX12_DEFINE_TYPE_GUID(class, CCryDX12GIAdapter, "59e4cc3a-87bb-4b7b-a1e3-06787231384a");
DX12_DEFINE_TYPE_GUID(class, CCryDX12GIFactory, "af688514-87ba-4c1c-9927-5a349ede6d74");
DX12_DEFINE_TYPE_GUID(class, CCryDX12GIOutput, "038b29e3-2d4b-49c0-a726-494675697b76");
DX12_DEFINE_TYPE_GUID(class, CCryDX12SwapChain, "d9e49be8-2651-4bc7-bfaa-303cb964a76e");
DX12_DEFINE_TYPE_GUID(class, CCryDX12Device, "f9c01631-d950-444c-a480-68bbb85fb28c");
DX12_DEFINE_TYPE_GUID(class, CCryDX12DeviceContext, "23ec965a-a01c-4862-b8b8-f53cf8cd7dcc");
DX12_DEFINE_TYPE_GUID(class, CCryDX12Texture1D, "dd8eb017-6cdb-4790-a767-52817c222d57");
DX12_DEFINE_TYPE_GUID(class, CCryDX12Texture2D, "4dff2102-e08a-497e-b519-1c07124b36b3");
DX12_DEFINE_TYPE_GUID(class, CCryDX12Texture3D, "003527af-f60d-4d93-b5d8-6370964e7692");
DX12_DEFINE_TYPE_GUID(class, CCryDX12DepthStencilView, "4fdfce9f-c7c9-44e7-99c5-64c24e5ed31a");
DX12_DEFINE_TYPE_GUID(class, CCryDX12RenderTargetView, "8b1d7ea0-0313-4eaa-8ddb-6e25b7eb7a7c");
DX12_DEFINE_TYPE_GUID(class, CCryDX12UnorderedAccessView, "ce7c421a-b6a5-42b2-a8cd-7a34bf23426b");
DX12_DEFINE_TYPE_GUID(class, CCryDX12ShaderResourceView, "b4639da3-fc4e-4907-942a-dae7e86e9eeb");
DX12_DEFINE_TYPE_GUID(class, CCryDX12Query, "3dd1b810-4a04-4719-bb8b-14f5936479f9");
DX12_DEFINE_TYPE_GUID(class, CCryDX12BlendState, "3bb119a1-5c56-4251-8fbf-c874d5d3f101");
DX12_DEFINE_TYPE_GUID(class, CCryDX12RasterizerState, "4c3c2198-2011-4dbd-a310-7dabca10e55d");
DX12_DEFINE_TYPE_GUID(class, CCryDX12DepthStencilState, "f28f8fb4-e55d-4f98-88ed-fa3aab12be7b");
DX12_DEFINE_TYPE_GUID(class, CCryDX12InputLayout, "3b74fa4e-c2b6-4167-a014-bf2f91f16ddd");
DX12_DEFINE_TYPE_GUID(class, CCryDX12Buffer, "06b2a225-e1aa-4509-b5da-f0fd1d846ba6");
DX12_DEFINE_TYPE_GUID(class, CCryDX12Shader, "7052b765-cc86-42e8-b116-36dc3bf4cfee");
DX12_DEFINE_TYPE_GUID(class, CCryDX12SamplerState, "a841545b-c5a6-4481-9383-a46ec1c2c380");
DX12_DEFINE_TYPE_GUID(class, CCryDX12EventQuery, "0d8f741f-4051-4935-a50d-58353f825e45");
DX12_DEFINE_TYPE_GUID(class, CCryDX12ResourceQuery, "6f1c2c43-3da7-489a-b1df-0c27362addbe");

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION CRYDX12GUID_HPP_SECTION_1
    #include AZ_RESTRICTED_FILE(CryDX12Guid_hpp)
#endif
