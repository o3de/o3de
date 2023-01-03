/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/EditorSplineComponentBus.h>
#include <LmbrCentral/Shape/EditorTubeShapeComponentBus.h>
#include <LmbrCentral/Shape/SplineComponentBus.h>

namespace AzToolsFramework
{
    class LinearManipulator;
}

namespace LmbrCentral
{
    /// ComponentMode designed for providing Viewport Editing of TubeShape.
    class EditorTubeShapeComponentMode
        : public AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode
        , private AZ::TransformNotificationBus::Handler
        , private ShapeComponentNotificationsBus::Handler
        , private SplineComponentNotificationBus::Handler
        , private EditorSplineComponentNotificationBus::Handler
        , private EditorTubeShapeComponentModeRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(EditorTubeShapeComponentMode, "{E186EBDF-29C7-4CE9-90C4-7A7E32349580}", EditorBaseComponentMode)

        /// Data required per TubeShape manipulator.
        struct TubeManipulatorState
        {
            AZ::SplineAddress m_splineAddress;
            AZ::u64 m_vertIndex;
        };

        EditorTubeShapeComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
        ~EditorTubeShapeComponentMode();

        static void Reflect(AZ::ReflectContext* context);

        static void RegisterActions();
        static void BindActionsToModes();
        static void BindActionsToMenus();

    private:
        // EditorBaseComponentMode
        void Refresh() override;
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl() override;
        AZStd::string GetComponentModeName() const override;
        AZ::Uuid GetComponentModeType() const override;

        // Manipulator handling
        void CreateManipulators();
        void DestroyManipulators();
        void ContainerChanged();

        // SplineComponentNotificationBus
        void OnSplineChanged() override;
        void OnVertexAdded(size_t index) override;
        void OnVertexRemoved(size_t index) override;
        void OnVerticesSet(const AZStd::vector<AZ::Vector3>& vertices) override;
        void OnVerticesCleared() override;
        void OnOpenCloseChanged(bool closed) override;

        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // EditorSplineComponentNotificationBus
        void OnSplineTypeChanged() override;

        // EditorTubeShapeComponentModeRequestBus overrides ...
        void ResetRadii() override;

        void RefreshManipulatorsLocal(AZ::EntityId entityId);

        AZ::Transform m_currentTransform; ///< The current localToWorld transform of the TubeShape.
        AZStd::vector<AZStd::shared_ptr<AzToolsFramework::LinearManipulator>> m_radiusManipulators; ///< Manipulators to control the radius (volume) of the tube at each vertex.
    };

    /// For a given Tube + Spline combo, generate data required for each manipulator at
    /// each vertex required for modifying the tube.
    AZStd::vector<EditorTubeShapeComponentMode::TubeManipulatorState> GenerateTubeManipulatorStates(const AZ::Spline& spline);
} // namespace LmbrCentral
