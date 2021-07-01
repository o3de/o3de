/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "EditorComponentBase.h"
#include "EditorDisabledCompositionBus.h"

namespace AzToolsFramework
{
    namespace Components
    {
        /**
        * Contains Disabled components to be added to the entity we are attached to.
        */
        class EditorDisabledCompositionComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , public EditorDisabledCompositionRequestBus::Handler
        {
        public:
            AZ_COMPONENT(EditorDisabledCompositionComponent, "{E77AE6AC-897D-4035-8353-637449B6DCFB}", EditorComponentBase);
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            ////////////////////////////////////////////////////////////////////
            // EditorDisabledCompositionRequestBus
            void GetDisabledComponents(AZStd::vector<AZ::Component*>& components) override;
            void AddDisabledComponent(AZ::Component* componentToAdd) override;
            void RemoveDisabledComponent(AZ::Component* componentToRemove) override;
            ////////////////////////////////////////////////////////////////////

            ~EditorDisabledCompositionComponent() override;
        private:
            ////////////////////////////////////////////////////////////////////
            // AZ::Entity
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////

            AZStd::vector<AZ::Component*> m_disabledComponents;
        };
    } // namespace Components
} // namespace AzToolsFramework
