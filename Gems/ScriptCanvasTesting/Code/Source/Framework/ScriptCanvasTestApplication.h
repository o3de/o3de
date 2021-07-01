/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Editor/Framework/ScriptCanvasGraphUtilities.h>
#include "EntityRefTests.h"

#include <Asset/EditorAssetSystemComponent.h>
#include <AzFramework/Application/Application.h>
#include <ScriptCanvas/SystemComponent.h>

namespace ScriptCanvasTests
{
    class Application
        : public AzFramework::Application
    {
    public:
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            AZ::ComponentTypeList components = AzFramework::Application::GetRequiredSystemComponents();
             components.insert(components.end(),
             {
                 azrtti_typeid<ScriptCanvasTests::TestComponent>(),
                 azrtti_typeid<ScriptCanvasEditor::TraceMessageComponent>(),
             });

            return components;
        }

        void CreateReflectionManager() override
        {
            AzFramework::Application::CreateReflectionManager();
            RegisterComponentDescriptor(ScriptCanvasTests::TestComponent::CreateDescriptor());
            RegisterComponentDescriptor(ScriptCanvasEditor::TraceMessageComponent::CreateDescriptor());
        }
    };
}

