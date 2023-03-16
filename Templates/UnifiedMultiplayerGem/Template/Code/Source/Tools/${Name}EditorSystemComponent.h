// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 // {END_LICENSE}

#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Unified/${Name}SystemComponent.h>

namespace ${SanitizedCppName}
{
    /// System component for ${SanitizedCppName} editor
    class ${SanitizedCppName}EditorSystemComponent
        : public ${SanitizedCppName}SystemComponent
        , protected AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = ${SanitizedCppName}SystemComponent;
    public:
        AZ_COMPONENT_DECL(${SanitizedCppName}EditorSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        ${SanitizedCppName}EditorSystemComponent();
        ~${SanitizedCppName}EditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace ${SanitizedCppName}
