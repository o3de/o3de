/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ScriptEventsBuilderWorker.h"

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <AzCore/Component/Component.h>

namespace ScriptEventsBuilder
{
    //! ScriptEventsBuilder is responsible for turning editor ScriptEvents Assets into runtime script canvas assets
    class ScriptEventsBuilderComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(ScriptEventsBuilderComponent, "{A402F019-0DD4-4CFF-B8A0-A90F818021E4}")
        static void Reflect(AZ::ReflectContext* context);

        ScriptEventsBuilderComponent() = default;
        ~ScriptEventsBuilderComponent() override = default;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

    private:
        ScriptEventsBuilderComponent(const ScriptEventsBuilderComponent&) = delete;

        Worker m_scriptEventsBuilder;
    };
}
