/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ScriptEventsSystemComponent.h"

#include <AzFramework/Application/Application.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
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
            RegisterComponentDescriptor(AZ::AssetManagerComponent::CreateDescriptor());
            RegisterComponentDescriptor(AzFramework::AssetCatalogComponent::CreateDescriptor());
        }
    };
}

