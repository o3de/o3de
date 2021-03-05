/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright EntityRef license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include "ScriptEventsSystemComponent.h"

#include <AzFramework/Application/Application.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzFramework/Asset/AssetCatalogComponent.h>

namespace ScriptEventsTests
{
    class Application
        : public AzFramework::Application
    {
    public:
        using SuperType = AzFramework::Application;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            AZ::ComponentTypeList components;
            components.insert(components.end(),
                {
                    azrtti_typeid<ScriptEvents::ScriptEventsSystemComponent>(),
                    azrtti_typeid<AZ::MemoryComponent>(),
                    azrtti_typeid<AZ::AssetManagerComponent>(),
                    azrtti_typeid<AZ::JobManagerComponent>(),
                    azrtti_typeid<AZ::StreamerComponent>(),
                    azrtti_typeid<AzFramework::AssetCatalogComponent>(),
                });

            return components;
        }

        void CreateReflectionManager() override
        {
            SuperType::CreateReflectionManager();
            RegisterComponentDescriptor(ScriptEvents::ScriptEventsSystemComponent::CreateDescriptor());
            RegisterComponentDescriptor(AZ::MemoryComponent::CreateDescriptor());
            RegisterComponentDescriptor(AZ::AssetManagerComponent::CreateDescriptor());
            RegisterComponentDescriptor(AzFramework::AssetCatalogComponent::CreateDescriptor());
        }
    };
}

