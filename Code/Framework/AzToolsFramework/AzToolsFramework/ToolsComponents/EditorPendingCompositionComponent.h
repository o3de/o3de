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