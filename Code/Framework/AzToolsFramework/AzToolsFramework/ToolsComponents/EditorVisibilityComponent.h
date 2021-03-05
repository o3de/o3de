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
