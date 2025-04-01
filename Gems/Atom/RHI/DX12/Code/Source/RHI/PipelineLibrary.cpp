/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Device.h>
#include <RHI/PipelineLibrary.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI.Profiler/GraphicsProfilerBus.h>

namespace AZ
{
    namespace DX12
    {
        namespace
        {
            // Builds a wide-character name from a 64-bit hash.
#if defined (AZ_DX12_USE_PIPELINE_LIBRARY)
            void HashToName(uint64_t hash, AZStd::wstring& name)
            {
                static const wchar_t s_hexValues[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

                const size_t nibbleCount = sizeof(hash) * 2;
                const size_t nibbleSize = 4;

                name.resize(nibbleCount);

                for (size_t nibbleIndex = 0; nibbleIndex < nibbleCount; ++nibbleIndex)
                {
                    uint64_t nibble = hash;
                    nibble >>= (nibbleIndex * nibbleSize);
                    nibble &= 0xF;
                    name[nibbleCount - nibbleIndex - 1] = s_hexValues[nibble];
                }
            }
#endif
        }

        RHI::Ptr<PipelineLibrary> PipelineLibrary::Create()
        {
            return aznew PipelineLibrary;
        }

        RHI::ResultCode PipelineLibrary::InitInternal(RHI::Device& deviceBase, [[maybe_unused]] const RHI::DevicePipelineLibraryDescriptor& descriptor)
        {
            Device& device = static_cast<Device&>(deviceBase);
            ID3D12DeviceX* dx12Device = device.GetDevice();

#if defined (AZ_DX12_USE_PIPELINE_LIBRARY)
            AZStd::span<const uint8_t> bytes;

            bool shouldCreateLibFromSerializedData = true;
            if (RHI::GraphicsProfilerBus::HasHandlers())
            {
                // CreatePipelineLibrary api does not function properly if Renderdoc or Pix is enabled
                shouldCreateLibFromSerializedData = false;
            }


            if (descriptor.m_serializedData && shouldCreateLibFromSerializedData)
            {
                bytes = descriptor.m_serializedData->GetData();
            }

            Microsoft::WRL::ComPtr<ID3D12PipelineLibraryX> libraryComPtr;

            if (!bytes.empty())
            {
                HRESULT hr = dx12Device->CreatePipelineLibrary(bytes.data(), bytes.size(), IID_GRAPHICS_PPV_ARGS(libraryComPtr.GetAddressOf()));

                if (SUCCEEDED(hr))
                {
                    m_serializedData = descriptor.m_serializedData;
                }
                else
                {
                    switch (hr)
                    {
                    case D3D12_ERROR_DRIVER_VERSION_MISMATCH:
                        AZ_Warning("PipelineLibrary", false, "Failed to use pipeline library blob due to driver version mismatch. Contents will be rebuilt.");
                        break;
                    case DXGI_ERROR_UNSUPPORTED:
                        AZ_Warning("PipelineLibrary", false, "Failed to use pipeline library blob due to the specified device interface or feature level not supported on this system. Contents will be rebuilt.");
                        break;
                    case D3D12_ERROR_ADAPTER_NOT_FOUND:
                        AZ_Warning("PipelineLibrary", false, "Failed to use pipeline library blob due to mismatched hardware. Contents will be rebuilt.");
                        break;
                    case E_INVALIDARG:
                        AZ_Assert(false, "Failed to use pipeline library blob due to invalid arguments. Contents will be rebuilt.");
                        break;
                    case DXGI_ERROR_DEVICE_REMOVED:
                        AZ_Assert(false, "Failed to use pipeline library blob due to DXGI_ERROR_DEVICE_REMOVED.");
                        device.OnDeviceRemoved();
                        break;
                    default:
                        AZ_Warning("PipelineLibrary", false, "Failed to use pipeline library blob for unknown reason. Contents will be rebuilt.");
                    }
                }
            }

            if (!libraryComPtr.Get())
            {
                // We didn't use serialized pipeline library blob, so need to create a fresh library.

                HRESULT hr = dx12Device->CreatePipelineLibrary(nullptr, 0, IID_GRAPHICS_PPV_ARGS(libraryComPtr.GetAddressOf()));

                if (FAILED(hr))
                {
                    return RHI::ResultCode::Fail;
                }
            }

            m_library = libraryComPtr.Get();
#endif

            m_dx12Device = dx12Device;

            return RHI::ResultCode::Success;
        }

        RHI::Ptr<ID3D12PipelineState> PipelineLibrary::CreateGraphicsPipelineState([[maybe_unused]] uint64_t hash, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineStateDesc)
        {
#if defined (AZ_DX12_USE_PIPELINE_LIBRARY)
            AZStd::wstring name;
            HashToName(hash, name);

            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

            Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineStateComPtr;
            HRESULT hr = m_library->LoadGraphicsPipeline(name.c_str(), &pipelineStateDesc, IID_GRAPHICS_PPV_ARGS(pipelineStateComPtr.GetAddressOf()));

            // Invalid Arg is returned if the entry doesn't exist.
            if (hr == E_INVALIDARG)
            {
                m_dx12Device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_GRAPHICS_PPV_ARGS(pipelineStateComPtr.GetAddressOf()));

                if (pipelineStateComPtr)
                {
                    hr = m_library->StorePipeline(name.c_str(), pipelineStateComPtr.Get());
                    if (!AssertSuccess(hr))
                    {
                        return nullptr;
                    }
                    m_pipelineStates.emplace(AZStd::move(name), pipelineStateComPtr.Get());
                }
            }
            else if (FAILED(hr))
            {
                return nullptr;
            }

            return pipelineStateComPtr.Get();
#else
            Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineStateComPtr;
            HRESULT hr = m_dx12Device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_GRAPHICS_PPV_ARGS(pipelineStateComPtr.GetAddressOf()));
            if (SUCCEEDED(hr))
            {
                return pipelineStateComPtr.Get();
            }
            return nullptr;
#endif
        }

        RHI::Ptr<ID3D12PipelineState> PipelineLibrary::CreateComputePipelineState([[maybe_unused]] uint64_t hash, const D3D12_COMPUTE_PIPELINE_STATE_DESC& pipelineStateDesc)
        {
#if defined (AZ_DX12_USE_PIPELINE_LIBRARY)
            AZStd::wstring name;
            HashToName(hash, name);

            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

            Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineStateComPtr;
            HRESULT hr = m_library->LoadComputePipeline(name.c_str(), &pipelineStateDesc, IID_GRAPHICS_PPV_ARGS(pipelineStateComPtr.GetAddressOf()));

            // Invalid Arg is returned if the entry doesn't exist.
            if (hr == E_INVALIDARG)
            {
                m_dx12Device->CreateComputePipelineState(&pipelineStateDesc, IID_GRAPHICS_PPV_ARGS(pipelineStateComPtr.GetAddressOf()));

                if (pipelineStateComPtr)
                {
                    hr = m_library->StorePipeline(name.c_str(), pipelineStateComPtr.Get());

                    if (!AssertSuccess(hr))
                    {
                        return nullptr;
                    }
                    m_pipelineStates.emplace(AZStd::move(name), pipelineStateComPtr.Get());
                }
            }
            else if (FAILED(hr))
            {
                return nullptr;
            }

            return pipelineStateComPtr.Get();
#else
            Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineStateComPtr;
            HRESULT hr = m_dx12Device->CreateComputePipelineState(&pipelineStateDesc, IID_GRAPHICS_PPV_ARGS(pipelineStateComPtr.GetAddressOf()));
            if (SUCCEEDED(hr))
            {
                return pipelineStateComPtr.Get();
            }
            return nullptr;
#endif
        }

        void PipelineLibrary::ShutdownInternal()
        {
#if defined (AZ_DX12_USE_PIPELINE_LIBRARY)
            m_library = nullptr;
            m_pipelineStates.clear();
#endif
        }

        RHI::ResultCode PipelineLibrary::MergeIntoInternal([[maybe_unused]] AZStd::span<const RHI::DevicePipelineLibrary* const> pipelineLibraries)
        {
            if (RHI::GraphicsProfilerBus::HasHandlers())
            {
                // StorePipeline api does not function properly if RenderDoc or Pix is enabled
                return RHI::ResultCode::Fail;
            }

#if defined (AZ_DX12_USE_PIPELINE_LIBRARY)
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
            for (const RHI::DevicePipelineLibrary* libraryBase : pipelineLibraries)
            {
                const PipelineLibrary* library = static_cast<const PipelineLibrary*>(libraryBase);
                for (const auto& pipelineStateEntry : library->m_pipelineStates)
                {
                    if (m_pipelineStates.find(pipelineStateEntry.first) == m_pipelineStates.end())
                    {
                        m_library->StorePipeline(pipelineStateEntry.first.c_str(), pipelineStateEntry.second.get());

                        m_pipelineStates.emplace(pipelineStateEntry.first, pipelineStateEntry.second);
                    }
                }
            }
#endif
            return RHI::ResultCode::Success;
        }

        RHI::ConstPtr<RHI::PipelineLibraryData> PipelineLibrary::GetSerializedDataInternal() const
        {
#if defined (AZ_DX12_USE_PIPELINE_LIBRARY)
            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

            AZStd::vector<uint8_t> serializedData(m_library->GetSerializedSize());
            if (serializedData.size())
            {
            
                HRESULT hr = m_library->Serialize(serializedData.data(), serializedData.size());

                if (!AssertSuccess(hr))
                {
                    return nullptr;
                }

                return RHI::PipelineLibraryData::Create(AZStd::move(serializedData));
            }
            return nullptr;
#else
            return nullptr;
#endif
        }

        bool PipelineLibrary::IsMergeRequired() const
        {
#if defined (AZ_DX12_USE_PIPELINE_LIBRARY)
            return !m_pipelineStates.empty();
#else
            return false;
#endif
        }

        bool PipelineLibrary::SaveSerializedDataInternal([[maybe_unused]] const AZStd::string& filePath) const
        {
            // DX12 drivers cannot save serialized data
            [[maybe_unused]] Device& device = static_cast<Device&>(GetDevice());
            AZ_Assert(!device.GetFeatures().m_isPsoCacheFileOperationsNeeded, "Explicit PSO cache operations should not be disabled for DX12");
            return false;
        }
    }
}
