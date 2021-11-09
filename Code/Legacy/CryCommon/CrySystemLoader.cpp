/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <CrySystemLoader.h>

#include <AzCore/Module/DynamicModuleHandle.h>
#include <CryCommon/ISystem.h>

constexpr const char* CrySystemLibraryName = AZ_TRAIT_OS_DYNAMIC_LIBRARY_PREFIX  "CrySystem" AZ_TRAIT_OS_DYNAMIC_LIBRARY_EXTENSION;

constexpr const char* CreateInterfaceFunctionName = "CreateSystemInterface";

constexpr const char* InjectEnvironmentFunctionName = "InjectEnvironment";
constexpr const char* DetachEnvironmentFunctionName = "DetachEnvironment";

using InjectEnvironmentFunction = void(*)(void*);
using DetachEnvironmentFunction = void(*)();

/* static */
AZStd::unique_ptr<CrySystemModuleHandle> CrySystemModuleHandle::Create()
{
    return AZStd::unique_ptr<CrySystemModuleHandle>(aznew CrySystemModuleHandle());
}
/* static */
const char* CrySystemModuleHandle::ModuleName()
{
    return CrySystemLibraryName;
}

CrySystemModuleHandle::~CrySystemModuleHandle()
{
    Unload();
}

bool CrySystemModuleHandle::Load()
{
    const bool loaded = m_crySystemHandle->Load(/*isInitializeFunctionRequired=*/false);
    if (loaded)
    {
        // We need to inject the environment first thing so that allocators are available immediately
        InjectEnvironmentFunction injectEnv = GetFunction<InjectEnvironmentFunction>(InjectEnvironmentFunctionName);
        if (injectEnv)
        {
            auto env = AZ::Environment::GetInstance();
            injectEnv(env);
        }

        m_createInterfaceFunc = GetFunction<CreateInterfaceFunction>(CreateInterfaceFunctionName);
    }
    return loaded;
}

bool CrySystemModuleHandle::Unload()
{
    if (m_crySystemHandle->IsLoaded())
    {
        DetachEnvironmentFunction detachEnv = GetFunction<DetachEnvironmentFunction>(DetachEnvironmentFunctionName);
        if (detachEnv)
        {
            detachEnv();
        }
    }
    return m_crySystemHandle->Unload();
}

ISystem* CrySystemModuleHandle::CreateSystemInterface(SSystemInitParams& initParams)
{
    if (m_createInterfaceFunc)
    {
        return m_createInterfaceFunc(initParams);
    }
    return nullptr;
}

CrySystemModuleHandle::CrySystemModuleHandle()
    : m_crySystemHandle(AZ::DynamicModuleHandle::Create(CrySystemLibraryName))
{
}
