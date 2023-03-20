
/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#pragma once

#include <MeshletsSystemComponent.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace AZ
{
    namespace Meshlets
    {
        /// System component for Meshlets editor
        class MeshletsEditorSystemComponent
            : public MeshletsSystemComponent
            , private AzToolsFramework::EditorEvents::Bus::Handler
        {
            using BaseSystemComponent = MeshletsSystemComponent;
        public:
            AZ_COMPONENT(MeshletsEditorSystemComponent, "{00c6370a-4390-41e4-aae3-a8425b2e776f}", BaseSystemComponent);
            static void Reflect(AZ::ReflectContext* context);

            MeshletsEditorSystemComponent();
            ~MeshletsEditorSystemComponent();

        private:
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

            // AZ::Component
            void Activate() override;
            void Deactivate() override;
        };
    } // namespace Meshlets
} // namespace AZ

