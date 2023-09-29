/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/Console/ConsoleTypeHelpers.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/Device/DeviceAttributeDeviceModel.h>
#include <AzFramework/Device/DeviceAttributeRAM.h>

#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")

namespace AzFramework
{
    AZStd::string QueryWMIProperty(IWbemServices* services, AZStd::string_view propertyName)
    {
        AZStd::string result;
        IEnumWbemClassObject* classObjectEnumerator = nullptr;

        // prepare a query to obtain the property value from Win32_ComputerSystem
        // https://learn.microsoft.com/en-us/windows/win32/cimwin32prov/win32-computersystem
        auto propertyQuery = AZStd::wstring::format(TEXT("SELECT %.*S FROM Win32_ComputerSystem"),
            AZ_STRING_ARG(propertyName));

        // ExecQuery expects BSTR types
        auto query = SysAllocString(propertyQuery.c_str());
        auto language = SysAllocString(TEXT("WQL"));
        auto hResult = services->ExecQuery(language, query,
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            nullptr,
            &classObjectEnumerator);
        SysFreeString(language);
        SysFreeString(query);

        if (!FAILED(hResult))
        {
            // prepare to enumerate the object, blocking until the objects are ready
            IWbemClassObject* classObject = nullptr;
            ULONG numResults = 0;
            hResult = classObjectEnumerator->Next(WBEM_INFINITE, 1, &classObject, &numResults);

            if (!FAILED(hResult) && classObject && numResults != 0)
            {
                // get the class object's property value
                VARIANT propertyValue;
                AZStd::wstring classObjectPropertyName;
                AZStd::to_wstring(classObjectPropertyName, propertyName);
                if (!FAILED(classObject->Get(classObjectPropertyName.c_str(), 0, &propertyValue, 0, 0)))
                {
                    if (propertyValue.vt != VT_NULL &&
                        propertyValue.vt != VT_EMPTY &&
                        !(propertyValue.vt & VT_ARRAY))
                    {
                        AZStd::to_string(result, propertyValue.bstrVal);
                    }
                }
                VariantClear(&propertyValue);
            }

            if (classObject)
            {
                classObject->Release();
            }
        }

        if (classObjectEnumerator)
        {
            classObjectEnumerator->Release();
        }

        return result;
    }

    AZStd::string GetWMIPropertyValue(AZStd::string_view propertyName)
    {
        AZStd::string propertyValue;

        // NOTE: this query functionality is not optimized for querying multiple values
        // If it becomes necessary to add more device attributes that must also query WMI
        // an optimization can be to query all the WMI properties for all attributes in a
        // single query, store in a static cache so they are available as the other device
        // attributes are created so they can pull the values from the cache.

        CoInitialize(nullptr);

        // Obtain the initial locator to Windows Management on a particular host computer.
        IWbemLocator* locator = nullptr;
        auto hResult = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&locator);
        if (!FAILED(hResult))
        {
            // Connect to the root\cimv2 namespace with the current user and obtain pointer pSvc to make IWbemServices calls.
            IWbemServices* services = nullptr;
            auto serverPath = SysAllocString(TEXT(R"(ROOT\CIMV2)"));
            hResult = locator->ConnectServer(serverPath, nullptr, nullptr, 0, NULL, 0, 0, &services);
            SysFreeString(serverPath);

            if (!FAILED(hResult))
            {
                // Set the IWbemServices proxy so that impersonation of the user (client) occurs.
                hResult = CoSetProxyBlanket( services,
                    RPC_C_AUTHN_WINNT,
                    RPC_C_AUTHZ_NONE,
                    nullptr,
                    RPC_C_AUTHN_LEVEL_CALL,
                    RPC_C_IMP_LEVEL_IMPERSONATE,
                    nullptr,
                    EOAC_NONE);

                if (!FAILED(hResult))
                {
                    propertyValue = QueryWMIProperty(services, propertyName);
                }
            }

            if (services)
            {
                services->Release();
            }
        }

        if (locator)
        {
            locator->Release();
        }

        CoUninitialize();

        return propertyValue;
    }

    DeviceAttributeDeviceModel::DeviceAttributeDeviceModel()
    {
        // Use WMI because the registry HARDWARE\DESCRIPTION\System\BIOS\SystemProductName is not
        // always available or non-empty and the name does not match what WMI returns for the Model
        m_value = GetWMIPropertyValue("Model");
    }

    DeviceAttributeRAM::DeviceAttributeRAM()
    {
        HMODULE kernel32Handle = ::GetModuleHandleW(L"Kernel32.dll");
        if (kernel32Handle)
        {
            using Func = BOOL(WINAPI*)(_Out_ LPMEMORYSTATUSEX lpMemStatus);
            auto globalMemoryStatusExFunc = reinterpret_cast<Func>(::GetProcAddress(kernel32Handle, "GlobalMemoryStatusEx"));
            if (globalMemoryStatusExFunc)
            {
                // OS RAM amount is returned in Bytes
                constexpr double bytesToGiB = 1024.0 * 1024.0 * 1024.0;
                MEMORYSTATUSEX memStats;
                memStats.dwLength = sizeof(memStats);
                if (globalMemoryStatusExFunc(&memStats))
                {
                    m_valueInGiB = aznumeric_cast<float>(static_cast<double>(memStats.ullTotalPhys) / bytesToGiB);
                    m_value = AZ::ConsoleTypeHelpers::ValueToString(m_valueInGiB);
                }
            }

            // fall back to oldest available method 
            if (m_valueInGiB == 0)
            {
                // OS RAM amount is returned in Bytes
                constexpr double bytesToGiB = 1024.0 * 1024.0 * 1024.0;
                MEMORYSTATUS memStats;
                memStats.dwLength = sizeof(memStats);
                ::GlobalMemoryStatus(&memStats);
                m_valueInGiB = aznumeric_cast<float>(static_cast<double>(memStats.dwTotalPhys) / bytesToGiB);
                m_value = AZ::ConsoleTypeHelpers::ValueToString(m_valueInGiB);
            }
        }
    }
} // AzFramework

