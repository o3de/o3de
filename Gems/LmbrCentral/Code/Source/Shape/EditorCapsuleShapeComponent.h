/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/Manipulators/CapsuleManipulatorRequestBus.h>
#include <AzToolsFramework/Manipulators/RadiusManipulatorRequestBus.h>
#include <AzToolsFramework/Manipulators/ShapeManipulatorRequestBus.h>
#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>
#include <Shape/CapsuleShapeComponent.h>
#include <Shape/EditorBaseShapeComponent.h>

namespace LmbrCentral
{
    class EditorCapsuleShapeComponent
        : public EditorBaseShapeComponent
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AzToolsFramework::CapsuleManipulatorRequestBus::Handler
        , private AzToolsFramework::RadiusManipulatorRequestBus::Handler
        , private AzToolsFramework::ShapeManipulatorRequestBus::Handler
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
            provided.push_back(AZ_CRC_CE("CapsuleShapeService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        // EditorComponentBase overrides ...
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // AZ::TransformNotificationBus overrides ...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

    private:
        AZ_DISABLE_COPY_MOVE(EditorCapsuleShapeComponent)

        // AzFramework::EntityDebugDisplayEventBus overrides ...
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        // AzToolsFramework::CapsuleManipulatorRequestBus overrides ...
        float GetHeight() const override;
        void SetHeight(float height) override;

        // AzToolsFramework::RadiusManipulatorRequestBus overrides ...
        float GetRadius() const override;
        void SetRadius(float radius) override;

        // AzToolsFramework::ShapeManipulatorRequestBus overrides ...
        AZ::Vector3 GetTranslationOffset() const override;
        void SetTranslationOffset(const AZ::Vector3& translationOffset) override;
        AZ::Transform GetManipulatorSpace() const override;
        AZ::Quaternion GetRotationOffset() const override;

        AZ::Crc32 ConfigurationChanged();
        void ClampHeight();
        void GenerateVertices();

        CapsuleShape m_capsuleShape; //!< Stores underlying capsule representation for this component.
        ShapeMesh m_capsuleShapeMesh; //!< Buffer to hold index and vertex data for CapsuleShape when drawing.

        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate
            m_componentModeDelegate; //!< Responsible for detecting ComponentMode activation and creating a concrete ComponentMode.
    };
} // namespace LmbrCentral
