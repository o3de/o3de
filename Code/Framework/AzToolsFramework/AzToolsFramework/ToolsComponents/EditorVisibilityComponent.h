/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "EditorVisibilityBus.h"
#include "EditorComponentBase.h"

namespace AzToolsFramework
{
    namespace Components
    {
        //! Controls whether an Entity is shown or hidden in the Editor.
        class EditorVisibilityComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , public EditorVisibilityRequestBus::Handler
        {
        public:
            AZ_COMPONENT(EditorVisibilityComponent, "{88E08E78-5C2F-4943-9F73-C115E6FFAB43}", EditorComponentBase);
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);

            ~EditorVisibilityComponent();

            // EditorVisibilityRequestBus ...
            void SetVisibilityFlag(bool flag) override;
            bool GetVisibilityFlag() override;

        private:
            // AZ::Entity ...
            void Init() override;

            bool m_visibilityFlag = true; //!< Whether this entity is individually set to be shown.
        };
    } // namespace Components
} // namespace AzToolsFramework
