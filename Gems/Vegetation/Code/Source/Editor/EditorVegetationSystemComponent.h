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

#include <AzCore/Component/Component.h>

namespace AzToolsFramework { class PropertyHandlerBase; }

namespace Vegetation
{
    /**
    * System component required to reflect the editor only classes until module level reflection is fixed.
    */
    class EditorVegetationSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(EditorVegetationSystemComponent, "{DC493537-8D9D-4088-943F-6FFE6D115F62}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        EditorVegetationSystemComponent() = default;
        ~EditorVegetationSystemComponent() = default;

        void Activate() override;
        void Deactivate() override;
    };

} // namespace Vegetation

