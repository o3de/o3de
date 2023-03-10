/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/Component/Component.h>
#include <AzCore/Module/Module.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Memory/Memory.h>
#include <ScriptEvents/ScriptEventsBus.h>
#include <ScriptEvents/ScriptEventSystem.h>

namespace ScriptEvents
{
    /**
    * The ScriptEvents::Module class coordinates with the application
    * to reflect classes and create system components.
    */
    class ScriptEventsModule
        : public AZ::Module
        , ScriptEvents::ScriptEventModuleConfigurationRequestBus::Handler
    {
    public:
        AZ_RTTI(ScriptEventsModule, "{DD54A1FE-2BDF-412C-AAB8-5A6BE01FE524}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ScriptEventsModule, AZ::SystemAllocator);

        ScriptEventsModule();
        virtual ~ScriptEventsModule()
        {
            ScriptEvents::ScriptEventModuleConfigurationRequestBus::Handler::BusDisconnect();
            delete m_systemImpl;
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;

        ScriptEventsSystemComponentImpl* GetSystemComponentImpl() override;

    private:

        ScriptEventsSystemComponentImpl* m_systemImpl;

    };
}
