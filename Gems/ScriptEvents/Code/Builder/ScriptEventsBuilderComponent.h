/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
