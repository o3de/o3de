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
#include <AzCore/Component/Component.h>
#include <${Name}/${Name}Bus.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace ${SanitizedCppName}
{
    /// System component for ${SanitizedCppName} editor
    class ${SanitizedCppName}EditorSystemComponent
        : public ${SanitizedCppName}RequestBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , public AZ::Component
    {
    public:
        AZ_COMPONENT(${SanitizedCppName}EditorSystemComponent, "${EditorSysCompClassId}");
        static void Reflect(AZ::ReflectContext* context);

        ${SanitizedCppName}EditorSystemComponent();
        ~${SanitizedCppName}EditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate();
        void Deactivate();
    };
} // namespace ${SanitizedCppName}
