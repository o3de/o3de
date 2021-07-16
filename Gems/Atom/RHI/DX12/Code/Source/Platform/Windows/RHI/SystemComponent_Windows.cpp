/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <RHI/SystemComponent.h>
#include <RHI/WindowsVersionQuery.h>

namespace AZ
{
    namespace DX12
    {
        // Required version of Windows in order to run the RHI DX12
        static constexpr WORD MinMajorVersion = 10;
        static constexpr WORD MinMinorVersion = 0;
        static constexpr WORD MinBuildVersion = 17763;

        bool SystemComponent::CheckSystemRequirements() const
        {
            // Since we are using DXIL for our shaders, we need to check for a minimum Windows version.
            // DXIL shaders are only supported from Windows 10 Creators Update.

            Platform::WindowsVersion windowsVersion;
            if (!GetWindowsVersion(&windowsVersion))
            {
                AZ_Warning("DX12", false, "Unable to query what Windows version the system is running on");
                return false;
            }

            if ((windowsVersion.m_majorVersion < MinMajorVersion) ||
                (windowsVersion.m_majorVersion == MinMajorVersion && windowsVersion.m_minorVersion < MinMinorVersion) ||
                (windowsVersion.m_majorVersion == MinMajorVersion && windowsVersion.m_minorVersion == MinMinorVersion && windowsVersion.m_buildVersion < MinBuildVersion))
            {
                AZ_Warning("DX12", false, "Current system Windows version (%d.%d.%d) does not meet minimum required version (%d.%d.%d)",
                    windowsVersion.m_majorVersion, windowsVersion.m_minorVersion, windowsVersion.m_buildVersion,
                    MinMajorVersion, MinMinorVersion, MinBuildVersion);
                return false;
            }

            // Check that we have at least one device that supports the D3D_FEATURE_LEVEL_12_0 level.
            Microsoft::WRL::ComPtr<IDXGIFactoryX> dxgiFactory;
            DX12::AssertSuccess(CreateDXGIFactory2(0, IID_GRAPHICS_PPV_ARGS(dxgiFactory.GetAddressOf())));

            Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
            bool foundCompatibleDevice = false;
            for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters(i, dxgiAdapter.ReleaseAndGetAddressOf()); ++i)
            {
                Microsoft::WRL::ComPtr<IDXGIAdapterX> dxgiAdapterX;
                dxgiAdapter->QueryInterface(IID_GRAPHICS_PPV_ARGS(dxgiAdapterX.GetAddressOf()));
                // Check to see if the adapter supports Direct3D 12,
                // but don't create the actual device.
                if (SUCCEEDED(D3D12CreateDevice(dxgiAdapterX.Get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
                {
                    foundCompatibleDevice = true;
                    break;
                }
            }

            return foundCompatibleDevice;
        }
    }
}
