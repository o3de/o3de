/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "platform.h"
#include "ISystem.h"
#include "CrySystemBus.h"

#include <AzCore/Module/Module.h>


/**
 * CryHooksModule is an AZ::Module with common hooks into CryEngine systems.
 * - Sets gEnv once CrySystem has initialized.
 * - Registers as a CrySystem event handler.
 */
class CryHooksModule
    : public AZ::Module
    , protected CrySystemEventBus::Handler
    , protected  ISystemEventListener
{
public:
    AZ_CLASS_ALLOCATOR(CryHooksModule, AZ::SystemAllocator)
    AZ_RTTI(CryHooksModule, "{BD896D16-6F7D-4EA6-A532-0A9E6BF3C089}", AZ::Module);
    CryHooksModule()
        : Module()
    {
        CrySystemEventBus::Handler::BusConnect();
    }

    ~CryHooksModule() override
    {
        CrySystemEventBus::Handler::BusDisconnect();

        if (gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
        {
            gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
        }
    }

protected:

    void OnCrySystemPreInitialize(ISystem& system, const SSystemInitParams& systemInitParams) override
    {
        (void)systemInitParams;

#if !defined(AZ_MONOLITHIC_BUILD)
        // When module is linked dynamically, we must set our gEnv pointer.
        // When module is linked statically, we'll share the application's gEnv pointer.
        gEnv = system.GetGlobalEnvironment();
#else
        (void)system;
#endif
    }

    void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams) override
    {
        (void)systemInitParams;

        system.GetISystemEventDispatcher()->RegisterListener(this);
    }

    void OnCrySystemPostShutdown() override
    {
#if !defined(AZ_MONOLITHIC_BUILD)
        gEnv = nullptr;
#endif
    }

    void OnSystemEvent(ESystemEvent /*event*/, UINT_PTR /*wparam*/, UINT_PTR /*lparam*/) override {}
};
