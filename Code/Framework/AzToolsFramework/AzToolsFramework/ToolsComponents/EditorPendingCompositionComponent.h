/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "EditorComponentBase.h"
#include "EditorPendingCompositionBus.h"

namespace AzToolsFramework
{
    namespace Components
    {
        /**
        * Contains pending components to be added to the entity we are attached to.
        */
        class EditorPendingCompositionComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , public EditorPendingCompositionRequestBus::Handler
        {
        public:
            AZ_COMPONENT(EditorPendingCompositionComponent, "{D40FCB35-153D-45B3-AF6D-7BA576D8AFBB}", EditorComponentBase);
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            ////////////////////////////////////////////////////////////////////
            // EditorPendingCompositionRequestBus
            void GetPendingComponents(AZStd::vector<AZ::Component*>& components) override;
            void AddPendingComponent(AZ::Component* componentToAdd) override;
            void RemovePendingComponent(AZ::Component* componentToRemove) override;
            ////////////////////////////////////////////////////////////////////

            ~EditorPendingCompositionComponent() override;
        private:
            ////////////////////////////////////////////////////////////////////
            // AZ::Entity
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////

            AZStd::vector<AZ::Component*> m_pendingComponents;
        };
    } // namespace Components
} // namespace AzToolsFramework
