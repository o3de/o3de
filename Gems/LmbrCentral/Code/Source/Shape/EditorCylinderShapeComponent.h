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
