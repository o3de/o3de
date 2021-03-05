/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
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
                 azrtti_typeid<ScriptCanvas::SystemComponent>(),
                 azrtti_typeid<ScriptCanvasTests::TestComponent>(),
                 azrtti_typeid<ScriptCanvasEditor::TraceMessageComponent>(),
             });

            return components;
        }

        void CreateReflectionManager() override
        {
            AzFramework::Application::CreateReflectionManager();
            RegisterComponentDescriptor(ScriptCanvas::SystemComponent::CreateDescriptor());
            RegisterComponentDescriptor(ScriptCanvasTests::TestComponent::CreateDescriptor());
            RegisterComponentDescriptor(ScriptCanvasEditor::TraceMessageComponent::CreateDescriptor());
        }
    };
}

