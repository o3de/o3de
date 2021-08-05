/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EditorBaseShapeComponent.h"
#include "QuadShapeComponent.h"
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <LmbrCentral/Shape/QuadShapeComponentBus.h>

namespace LmbrCentral
{
    //! Editor representation of a rectangle in 3d space.
    class EditorQuadShapeComponent
        : public EditorBaseShapeComponent
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(LmbrCentral::EditorQuadShapeComponent, EditorQuadShapeComponentTypeId, EditorBaseShapeComponent);
        static void Reflect(AZ::ReflectContext* context);

        EditorQuadShapeComponent() = default;

        //! AZ::Component overrides...
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        //! EditorComponentBase overrides...
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    private:
        AZ_DISABLE_COPY_MOVE(EditorQuadShapeComponent);

        //! AzFramework::EntityDebugDisplayEventBus overrides...
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        void ConfigurationChanged();

        QuadShape m_quadShape; //! Stores underlying quad representation for this component.
    };
} // namespace LmbrCentral
