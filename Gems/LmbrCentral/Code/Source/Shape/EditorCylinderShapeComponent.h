/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "EditorBaseShapeComponent.h"
#include "CylinderShapeComponent.h"
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <LmbrCentral/Shape/CylinderShapeComponentBus.h>

namespace LmbrCentral
{
    class EditorCylinderShapeComponent
        : public EditorBaseShapeComponent
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorCylinderShapeComponent, EditorCylinderShapeComponentTypeId, EditorBaseShapeComponent);
        static void Reflect(AZ::ReflectContext* context);

        EditorCylinderShapeComponent() = default;

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            EditorBaseShapeComponent::GetProvidedServices(provided);
            provided.push_back(AZ_CRC("CylinderShapeService", 0x507c688e));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        AZ_DISABLE_COPY_MOVE(EditorCylinderShapeComponent)

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        void ConfigurationChanged();

        CylinderShape m_cylinderShape; ///< Stores underlying cylinder representation for this component.
    };
} // namespace LmbrCentral
