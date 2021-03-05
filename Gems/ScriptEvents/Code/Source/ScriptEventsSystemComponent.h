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
#pragma once


#include <AzCore/Component/Component.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>

#include <ScriptEvents/ScriptEvent.h>
#include <ScriptEvents/ScriptEventsBus.h>
#include <ScriptEvents/ScriptEventsAsset.h>
#include <ScriptEvents/ScriptEventDefinition.h>
#include <ScriptEvents/ScriptEventSystem.h>

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
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        // Script Event Assets
        AZStd::unordered_map<ScriptEventKey, AZStd::intrusive_ptr<ScriptEvents::Internal::ScriptEvent>> m_scriptEvents;

    };


    class ScriptEventsSystemComponentRuntimeImpl
        : public ScriptEventsSystemComponentImpl
    {
    public:

        void RegisterAssetHandler() override;
        void UnregisterAssetHandler() override;

        AZStd::unique_ptr<ScriptEventAssetRuntimeHandler> m_assetHandler;
    };

}


