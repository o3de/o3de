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

#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/API/EntityCompositionNotificationBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Ai/NavigationAreaBus.h>
#include <AzCore/Component/TickBus.h>

// ISerialize.h required by INavigationSystem.h
#include <ISerialize.h>
#include <INavigationSystem.h>

namespace AZ
{
    class PolygonPrism;
    class SerializeContext;
    class Vector3;
}

namespace LmbrCentral
{
    /**
     * EditorNavigationAreaComponent makes use of PolygonPrismShape to construct a volume to generate
     * a Nav Mesh for the terrain to be used by AI characters for navigation.
     */
    class EditorNavigationAreaComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public INavigationSystem::INavigationSystemListener
        , private ShapeComponentNotificationsBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private NavigationAreaRequestBus::Handler
        , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
        , private AzToolsFramework::EntityCompositionNotificationBus::Handler
        , private AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(EditorNavigationAreaComponent, "{8391FF77-7F4E-4576-9617-37793F88C5DA}", AzToolsFramework::Components::EditorComponentBase);

        EditorNavigationAreaComponent();
        ~EditorNavigationAreaComponent() override;

    protected:
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

    private:
        // ShapeComponentNotificationsBus::Handler
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // NavigationAreaRequestBus
        void RefreshArea() override;

        // INavigationSystemListener
        void OnNavigationEvent(const INavigationSystem::ENavigationEvent event) override;

        // EditorNavigationAreaComponent
        void UpdateGameArea();
        void RelinkWithMesh(bool updateGameArea);
        void UpdateMeshes();
        void ApplyExclusion();
        void DestroyVolume();
        void DestroyMeshes();
        void CreateVolume(AZ::Vector3* vertices, size_t vertexCount, NavigationVolumeID requestedID);
        void DestroyArea();
        
        // EditorEntityContextNotificationBus
        void OnStartPlayInEditorBegin() override;

        // EntityCompositionNotificationBus 
        void OnEntityCompositionChanging(const AzToolsFramework::EntityIdList& entityIds) override;
        void OnEntityCompositionChanged(const AzToolsFramework::EntityIdList& entityIds) override;

        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("NavigationAreaService", 0xd6ec6566));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("PolygonPrismShapeService", 0x1cbc4ed4));
        }

        static void Reflect(AZ::ReflectContext* context);

        /**
         * Called when editor property grid values are modified to ensure navigation area updates correctly.
         */
        void OnNavigationAreaChanged() const
        {
            if (m_navigationAreaChanged)
            {
                m_navigationAreaChanged();
            }
        }

        AZStd::vector<AZStd::string> m_agentTypes; ///< Define a list of AgentTypes corresponding to those defined in Scripts/AI/Navigation.xml.
        AZStd::vector<AZ::u32> m_meshes; ///< NavigationMeshID - vector of mesh ids for each AgentType.
        AZStd::string m_name; ///< Name used to register volume (currently Entity name).
        AZ::u32 m_volume = 0; ///< NavigationVolumeID - id of created nav mesh volume.
        bool m_exclusion = false; ///< Is this area an exclusion volume or not (should it add or subtract from the nav mesh).
        AZStd::function<void()> m_navigationAreaChanged = nullptr; ///< Callback when the navigation area is modified.

        bool m_switchingToGameMode = false; ///< Set if GameView was started so we know not to destroy navigation areas in Deactivate.
        bool m_compositionChanging = false; ///< Set if composition is changing so we know not to destroy navigation areas while scrubbing.
    };
} // namespace LmbrCentral