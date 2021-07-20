/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "EditorBaseShapeComponent.h"
#include "CapsuleShapeComponent.h"
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>

namespace LmbrCentral
{
    class EditorCapsuleShapeComponent
        : public EditorBaseShapeComponent
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorCapsuleShapeComponent, EditorCapsuleShapeComponentTypeId, EditorBaseShapeComponent);
        static void Reflect(AZ::ReflectContext* context);

        EditorCapsuleShapeComponent() = default;

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            EditorBaseShapeComponent::GetProvidedServices(provided);
            provided.push_back(AZ_CRC("CapsuleShapeService", 0x9bc1122c));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        AZ_DISABLE_COPY_MOVE(EditorCapsuleShapeComponent)

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        AZ::Crc32 ConfigurationChanged();
        void ClampHeight();
        void GenerateVertices();

        CapsuleShape m_capsuleShape; ///< Stores underlying capsule representation for this component.
        ShapeMesh m_capsuleShapeMesh; ///< Buffer to hold index and vertex data for CapsuleShape when drawing.
    };
} // namespace LmbrCentral
