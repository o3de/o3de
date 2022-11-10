/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "AxisAlignedBoxShape.h"
#include "AxisAlignedBoxShapeComponent.h"
#include "EditorBaseShapeComponent.h"
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/Manipulators/BoxManipulatorRequestBus.h>

namespace LmbrCentral
{
    /// Editor representation of Box Shape Component.
    class EditorAxisAlignedBoxShapeComponent
        : public EditorBaseShapeComponent
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AzToolsFramework::BoxManipulatorRequestBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorAxisAlignedBoxShapeComponent, EditorAxisAlignedBoxShapeComponentTypeId, EditorBaseShapeComponent);
        static void Reflect(AZ::ReflectContext* context);

        EditorAxisAlignedBoxShapeComponent() = default;

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // BoundsRequestBus overrides ...
        AZ::Aabb GetLocalBounds() override;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // AZ::TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

    private:
        AZ_DISABLE_COPY_MOVE(EditorAxisAlignedBoxShapeComponent)

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        // AzToolsFramework::BoxManipulatorRequestBus
        AZ::Vector3 GetDimensions() override;
        void SetDimensions(const AZ::Vector3& dimensions) override;
        AZ::Transform GetCurrentTransform() override;
        AZ::Transform GetCurrentLocalTransform() override;
        AZ::Vector3 GetBoxScale() override;

        void ConfigurationChanged();        

        AxisAlignedBoxShape m_aaboxShape; ///< Stores underlying box representation for this component.
        
        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate m_componentModeDelegate; /**< Responsible for detecting ComponentMode activation
                                                         *  and creating a concrete ComponentMode.*/
    };
} // namespace LmbrCentral
