/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Module/DynamicModuleHandle.h>
#include <CryCommon/ISystem.h>


class CrySystemModuleHandle
{
public:
    AZ_CLASS_ALLOCATOR(CrySystemModuleHandle, AZ::OSAllocator, 0);

    static AZStd::unique_ptr<CrySystemModuleHandle> Create();
    static const char* ModuleName();

    ~CrySystemModuleHandle();

    bool Load();
    bool Unload();

    ISystem* CreateSystemInterface(SSystemInitParams& initParams);

private:
    CrySystemModuleHandle();

    template<typename Function>
    Function GetFunction(const char* functionName) const
    {
        return m_crySystemHandle->GetFunction<Function>(functionName);
    }

    AZStd::unique_ptr<AZ::DynamicModuleHandle> m_crySystemHandle;

    using CreateInterfaceFunction = ISystem*(*)(SSystemInitParams& initParams);
    CreateInterfaceFunction m_createInterfaceFunc{ nullptr };
};
