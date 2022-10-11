/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <RecastNavigationSystemComponent.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace RecastNavigation
{
    //! System component for RecastNavigation editor.
    class RecastNavigationEditorSystemComponent
        : public RecastNavigationSystemComponent
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = RecastNavigationSystemComponent;
    public:
        AZ_COMPONENT(RecastNavigationEditorSystemComponent, "{2f0e450d-6ded-4e92-952a-4aa855fdfff8}", BaseSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

        RecastNavigationEditorSystemComponent();
        ~RecastNavigationEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        //! AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;
    };
} // namespace RecastNavigation
