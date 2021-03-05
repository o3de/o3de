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
#include "precompiled.h"

#include "ScriptEventsSystemComponent.h"

#include <AzCore/Memory/SystemAllocator.h>

#include <ScriptEvents/ScriptEventsGem.h>

#include <ScriptEvents/Components/ScriptEventReferencesComponent.h>

namespace ScriptEvents
{
    ScriptEventsModule::ScriptEventsModule()
        : AZ::Module()
        , m_systemImpl(nullptr)
    {
        ScriptEventModuleConfigurationRequestBus::Handler::BusConnect();

        m_descriptors.insert(m_descriptors.end(), {
            ScriptEvents::ScriptEventsSystemComponent::CreateDescriptor(),
            ScriptEvents::Components::ScriptEventReferencesComponent::CreateDescriptor(),
        });
    }

    ScriptEventsSystemComponentImpl* ScriptEventsModule::GetSystemComponentImpl()
    {
        if (!m_systemImpl)
        {
            m_systemImpl = aznew ScriptEventsSystemComponentRuntimeImpl();
        }

        return m_systemImpl;
    }

    /**
    * Add required SystemComponents to the SystemEntity.
    */
    AZ::ComponentTypeList ScriptEventsModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<ScriptEvents::ScriptEventsSystemComponent>(),
        };
    }
}

AZ_DECLARE_MODULE_CLASS(Gem_ScriptEvents, ScriptEvents::ScriptEventsModule)
