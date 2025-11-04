/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once


#include <AzCore/Component/Component.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Debug/Budget.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>

#include <ScriptEvents/ScriptEvent.h>
#include <ScriptEvents/ScriptEventsBus.h>
#include <ScriptEvents/ScriptEventsAsset.h>
#include <ScriptEvents/ScriptEventDefinition.h>
#include <ScriptEvents/ScriptEventSystem.h>

AZ_DECLARE_BUDGET(ScriptCanvas);

namespace ScriptEvents
{
   class ScriptEventsSystemComponent
        : public AZ::Component
    {
    public:

        AZ_COMPONENT(ScriptEventsSystemComponent, "{43068F27-B171-4DF4-B583-57CEF3F2AC6C}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component...
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        // Script Event Assets
        AZStd::unordered_map<ScriptEventKey, AZStd::intrusive_ptr<ScriptEvents::Internal::ScriptEventRegistration>> m_scriptEvents;

    };


    class ScriptEventsSystemComponentRuntimeImpl
        : public ScriptEventsSystemComponentImpl
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptEventsSystemComponentRuntimeImpl, AZ::SystemAllocator)

        void RegisterAssetHandler() override;
        void UnregisterAssetHandler() override;

        AZStd::unique_ptr<ScriptEventAssetRuntimeHandler> m_assetHandler;
    };

}


