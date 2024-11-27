/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <PhysXDebug/PhysXDebugBus.h>
#include <PxPhysicsAPI.h>

#include <CryCommon/CrySystemBus.h>

#include <AzFramework/Physics/SystemBus.h>

#ifdef IMGUI_ENABLED
#include <imgui/imgui.h>
#include <ImGuiBus.h>
#endif // #ifdef IMGUI_ENABLED

namespace PhysXDebug
{
    struct PhysXVisualizationSettings
    {
        AZ_RTTI(PhysXVisualizationSettings, "{A3A03872-36A3-44AB-B0A9-29F709E8E3B0}");

        virtual ~PhysXVisualizationSettings() = default;

        bool m_visualizationEnabled = false;
        bool m_visualizeCollidersByProximity = false;

        // physx culling only applied to: eCOLLISION_SHAPES, eCOLLISION_EDGES and eCOLLISION_FNORMALS (eCOLLISION_AABBS are not culled by PhysX!)
        // see: \PhysX_3.4\Source\PhysX\src\NpShapeManager.cpp
        float m_scale = 1.0f;
        bool m_collisionShapes = true;
        bool m_collisionEdges = true;
        bool m_collisionFNormals = false;

        // the remaining properties will start *disable*
        bool m_collisionAabbs = false;
        bool m_collisionAxes = false;
        bool m_collisionCompounds = false;
        bool m_collisionStatic = false;
        bool m_collisionDynamic = false;

        bool m_bodyAxes = false;
        bool m_bodyMassAxes = false;
        bool m_bodyLinVelocity = false;
        bool m_bodyAngVelocity = false;

        bool m_contactPoint = false;
        bool m_contactNormal = false;

        bool m_jointLocalFrames = false;
        bool m_jointLimits = false;

        bool m_mbpRegions = false;
        bool m_actorAxes = false;

        /// Determine if the PhysX Debug Gem Visualization is currently enabled (for the editor context)
        inline bool IsPhysXDebugEnabled() { return m_visualizationEnabled; };
    };

    struct Culling
    {
        AZ_RTTI(Culling, "{20727A63-4FF7-4F31-B6F5-7FEFCB7CB153}");

        virtual ~Culling() = default;

        bool m_enabled = true;
        bool m_boxWireframe = false;
        float m_boxSize = 35.0f;
    };

    struct ColorMappings
    {
        AZ_RTTI(ColorMappings, "{021E40A6-568E-430A-9332-EF180DACD3C0}");
        // user defined colors for physx debug primitives
        AZ::Color m_defaultColor;
        AZ::Color m_black;
        AZ::Color m_red;
        AZ::Color m_green;
        AZ::Color m_blue;
        AZ::Color m_yellow;
        AZ::Color m_magenta;
        AZ::Color m_cyan;
        AZ::Color m_white;
        AZ::Color m_grey;
        AZ::Color m_darkRed;
        AZ::Color m_darkGreen;
        AZ::Color m_darkBlue;
    };

    class SystemComponent
        : public AZ::Component
        , protected PhysXDebugRequestBus::Handler
        , public AZ::TickBus::Handler
        , public CrySystemEventBus::Handler
#ifdef IMGUI_ENABLED
        , public ImGui::ImGuiUpdateListenerBus::Handler
#endif // IMGUI_ENABLED
    {
    public:

        AZ_COMPONENT(SystemComponent, "{111041CE-4C75-48E0-87C3-20938C05B9E0}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        SystemComponent();

        // PhysxDebugDraw::PhysxDebugDrawRequestBus
        void SetVisualization(bool enabled) override;
        void ToggleVisualizationConfiguration() override;
        void SetCullingBoxSize(float cullingBoxSize) override;
        void ToggleCullingWireFrame() override;
        void ToggleColliderProximityDebugVisualization() override;

#ifdef IMGUI_ENABLED
        void OnImGuiMainMenuUpdate() override;
#endif // IMGUI_ENABLED

    protected:

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override { return AZ::ComponentTickBus::TICK_FIRST + 1; }

        // CrySystemEvents
        void OnCrySystemInitialized(ISystem&, const SSystemInitParams&) override;

    private:

        /// Configure a PhysX scene debug visualization properties.
        void ConfigurePhysXVisualizationParameters();

        /// Convert from PhysX Visualization debug colors to user defined colors.
        /// @param originalColor a color from the PhysX debug visualization data.
        /// @return a user specified color mapping (defaulting to the original PhysX color).
        AZ::Color MapOriginalPhysXColorToUserDefinedValues(const physx::PxU32& originalColor);

        /// Initialise the PhysX debug draw colors based on defaults.
        void InitPhysXColorMappings();

        /// Draw the culling box being used by the viewport.
        /// @param cullingBoxAabb culling box Aabb to debug draw.
        void DrawDebugCullingBox(const AZ::Aabb& cullingBoxAabb);

        /// Configure primary debug draw settings for PhysX
        /// @param context the reflect context to utilize.
        static void ReflectPhysXDebugSettings(AZ::ReflectContext* context);

        /// Configure a culling box for PhysX visualization from the active camera.
        void ConfigureCullingBox();

        /// Gather visualization lines for this scene.
        void GatherLines(const physx::PxRenderBuffer& rb);

        /// Gather visualization triangles for this scene.
        void GatherTriangles(const physx::PxRenderBuffer& rb);

        /// Gather Joint Limits.
        void GatherJointLimits();

        /// Helper functions to wrap buffer management functionality.
        void ClearBuffers();
        void GatherBuffers();
        void RenderBuffers();

        /// Updates PhysX preferences to perform collider visualization based on proximity to camera.
        void UpdateColliderVisualizationByProximity();

#ifdef IMGUI_ENABLED
        /// Build a specific color picker menu option.
        void BuildColorPickingMenuItem(const AZStd::string& label, AZ::Color& color);
#endif // IMGUI_ENABLED

        physx::PxScene* GetCurrentPxScene();

        // Main configuration
        PhysXVisualizationSettings m_settings;
        Culling m_culling;
        ColorMappings m_colorMappings;
        AZ::ScriptTimePoint m_currentTime;
        bool m_registered = false;
        physx::PxBounds3 m_cullingBox;
        bool m_editorPhysicsSceneDirty = true;
        static const float m_maxCullingBoxSize;

        AZStd::vector<AZ::Vector3> m_linePoints;
        AZStd::vector<AZ::Color> m_lineColors;
        AZStd::vector<AZ::Vector3> m_trianglePoints;
        AZStd::vector<AZ::Color> m_triangleColors;

        // joint limit buffers
        AZStd::vector<AZ::Vector3> m_jointVertexBuffer;
        AZStd::vector<AZ::u32> m_jointIndexBuffer;
        AZStd::vector<AZ::Vector3> m_jointLineBuffer;
        AZStd::vector<bool> m_jointLineValidityBuffer;

        AzPhysics::SceneEvents::OnSceneSimulationFinishHandler m_sceneFinishSimHandler;
    };

    /// Possible console parameters for physx_Debug cvar.
    enum class DebugCVarValues : uint8_t
    {
        Disable, ///< Disable debug visualization.
        Enable, ///< Enable debug visualization.
        SwitchConfigurationPreference, ///< Switch between basic and full visualization configuration.
        ColliderProximityDebug ///< Toggle visualize collision shapes by proximity to camera in editor mode.
    };
}
