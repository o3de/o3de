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

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class EditorPrefabComponent : public AzToolsFramework::Components::EditorComponentBase
        {
        public:
            AZ_COMPONENT(EditorPrefabComponent, "{756E5F9C-3E08-4F8D-855C-A5AEEFB6FCDD}", EditorComponentBase);

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);

        private:
            void Activate() override;
            void Deactivate() override;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
