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
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Editor/AWSCoreEditorManager.h>

namespace AWSCore
{
    class AWSCoreEditorSystemComponent
        : public AZ::Component
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
    public:
        static constexpr const char EDITOR_HELP_MENU_TEXT[] = "&Help";

        AZ_COMPONENT(AWSCoreEditorSystemComponent, "{6098B19B-90F2-41DC-8D01-70277980249D}");

        AWSCoreEditorSystemComponent() = default;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    private:
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // AzToolsFramework::EditorEvents interface implementation
        void NotifyMainWindowInitialized(QMainWindow* mainWindow) override;

        AZStd::unique_ptr<AWSCoreEditorManager> m_awsCoreEditorManager;
    };

} // namespace AWSCore
