/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

