/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/EBus/Event.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzFramework/Physics/Material.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <AzFramework/Physics/Configuration/SystemConfiguration.h>
#include <AzFramework/Physics/Configuration/SceneConfiguration.h>

namespace AzPhysics
{
    //!Interface to access the Physics System.
    //! 
    class SystemInterface
    {
    public:
        AZ_RTTI(SystemInterface, "{B6F4D92A-061B-4CB3-AAB5-984B599A53AE}");

        SystemInterface() = default;
        virtual ~SystemInterface() = default;
        AZ_DISABLE_COPY_MOVE(SystemInterface);

        //! Initialize the Physics system with the given configuration.
        //! @param config Contains the configuration options
        virtual void Initialize(const SystemConfiguration* config) = 0;
        //! Will re-initialize the physics backend.
        //! Will preserve Scene and Simulation Body data along with any existing Handles.
        virtual void Reinitialize() = 0;
        //! Teardown the whole Physics system.
        //! This removes all Scene and Simulation Body data, then physics will stop running.
        virtual void Shutdown() = 0;

        //! Advance the Physics state.
        //! This will iterate the Scene list, update All simulation bodies and advance the physics state by the given delta time.
        //! It is recommended to call this function to run the physics tick. Advanced users may manually update each scene, if required to have finer control.
        //! This function will also signal the OnPresimulateEvent and OnPostsimulateEvent. The OnPresimulateEvent will have a parameter that will be the total time executed by the simulation.
        //! When SystemConfiguration::m_fixedTimestep is greater than zero, the simulation will run at the fixed time step and may run multiple steps per frame.
        //! This time can range from 0.0f to SystemConfiguration::m_maxTimestep. Where 0.0 time indicates that the simulation did not execute any steps this frame.
        //! When SystemConfiguration::m_fixedTimestep equal to or less than zero, the simulation will step once with a time between deltaTime and SystemConfiguration::m_maxTimestep.
        //! Example Advanced users might do to self manage Advancing the physics state.
        //! @code{ .cpp }
        //! //The following would be somewhere in the client code game update loop.
        //! if (auto* system = AZ::Interface<AzPhysics::SystemInterface>::Get())
        //! {
        //!     SceneInterfaceList& sceneList = system->GetAllScenes();
        //!     //Here you may want to order the update of the scenes, or only update specific scenes.
        //!     for(auto scene : sceneList)
        //!     {
        //!         //StartSimulation + FinishSimulation must be called in order.
        //!         scene->StartSimulation(deltatime); //spawns physics jobs.
        //!
        //!         //here you can perform other actions that do not rely on up to date physics.
        //!
        //!         scene->FinishSimulation();  //blocks until jobs are complete and swap buffers.
        //!     }
        //! }
        //! @endcode
        //! @param deltaTime This is the time in seconds to simulate physics for this tick (60fps = 0.01666667). Typically the frame deltaTime of the game loop.
        virtual void Simulate(float deltaTime) = 0;

        //! Add a scene to the physics simulation.
        //! @param config This is the Configuration of the scene to add.
        //! @return Returns a SceneHandle of the Scene created or InvalidSceneHandle if it fails.
        virtual SceneHandle AddScene(const SceneConfiguration& config) = 0;

        //! Add multiple scenes to the physics simulation.
        //! @param configs This is the list of SceneConfiguration objects to add.
        //! @return Returns a list of SceneHandle objects for each created Scene. Order will be the same as the SceneConfigurationList provided.
        virtual SceneHandleList AddScenes(const SceneConfigurationList& configs) = 0;

        //! Get the Scene of the requested SceneHandle.
        //! @param handle The SceneHandle of the requested scene.
        //! @return Returns a SceneInterface pointer if found, otherwise nullptr.
        virtual Scene* GetScene(SceneHandle handle) = 0;

        //! Get multiple Scenes.
        //! @param handles A list of SceneHandle objects to retrieve.
        //! @returns Returns a list of SceneInterface pointers. The order is the same as supplied. Pointer may be null if not a valid SceneHandle.
        virtual SceneList GetScenes(const SceneHandleList& handles) = 0;

        //! Retrieve all current Scenes.
        //! @return Returns a list of SceneInterface pointers.
        virtual SceneList& GetAllScenes() = 0;

        //! Remove the requested Scene if it exists.
        //! @param handle The handle to the scene to remove.
        virtual void RemoveScene(SceneHandle handle) = 0;

        //! Remove many Scenes if they exist.
        //! @param handles A list of handles to each scene to remove.
        virtual void RemoveScenes(const SceneHandleList& handles) = 0;

        //! Removes All Scenes.
        virtual void RemoveAllScenes() = 0;

        //! Get the current SystemConfiguration used to initialize the Physics system.
        virtual const SystemConfiguration* GetConfiguration() const = 0;

        //! Update the SystemConfiguration.
        //! This will apply the new configuration, some properties may require the reinitialization of the physics system and will tear down all Scenes and Simulation bodies.
        //! @param newConfig The new configuration to apply.
        //! @param forceReinitialization Flag to force a reinitialization of the physics system. Default false.
        virtual void UpdateConfiguration(const SystemConfiguration* newConfig, bool forceReinitialization = false) = 0;

        //! Update the default material library.
        //! @param materialLibrary The new material library asset to use.
        virtual void UpdateDefaultMaterialLibrary(const AZ::Data::Asset<Physics::MaterialLibraryAsset>& materialLibrary) = 0;

        //! Accessor to get the current Material Library. This is also available in the PhysXSystemConfiguration.
        virtual const AZ::Data::Asset<Physics::MaterialLibraryAsset>& GetDefaultMaterialLibrary() const = 0;

        //! Update the current default scene configuration.
        //! This is the configuration used to to create scenes without a custom configuration.
        //! @param sceneConfiguration The new configuration to apply.
        virtual void UpdateDefaultSceneConfiguration(const SceneConfiguration& sceneConfiguration) = 0;

        //! Gets the current default scene configuration.
        virtual const SceneConfiguration& GetDefaultSceneConfiguration() const = 0;

        //! Register to receive notifications when the Physics System is Initialized.
        //! @param handler The handler to receive the event.
        void RegisterSystemInitializedEvent(SystemEvents::OnInitializedEvent::Handler& handler) { handler.Connect(m_initializeEvent); }
        //! Register to receive notifications when the Physics System is reinitialized.
        //! @param handler The handler to receive the event.
        void RegisterSystemReInitializedEvent(SystemEvents::OnReinitializedEvent::Handler& handler) { handler.Connect(m_reinitializeEvent); }
        //! Register to receive notifications when the Physics System shuts down.
        //! @param handler The handler to receive the event.
        void RegisterSystemShutdownEvent(SystemEvents::OnShutdownEvent::Handler& handler) { handler.Connect(m_shutdownEvent); }
        //! Register to receive notifications when the Physics System simulation begins.
        //! @param handler The handler to receive the event.
        void RegisterPreSimulateEvent(SystemEvents::OnPresimulateEvent::Handler& handler) { handler.Connect(m_preSimulateEvent); }
        //! Register to receive notifications when the Physics System simulation ends.
        //! @param handler The handler to receive the event.
        void RegisterPostSimulateEvent(SystemEvents::OnPostsimulateEvent::Handler& handler) { handler.Connect(m_postSimulateEvent); }
        //! Register to receive notifications when the SystemConfiguration changes.
        //! @param handler The handler to receive the event.
        void RegisterSystemConfigurationChangedEvent(SystemEvents::OnConfigurationChangedEvent::Handler& handler) { handler.Connect(m_configChangeEvent); }
        //! Register a handler to receive an event when the default material library changes.
        //! @param handler The handler to receive the event.
        void RegisterOnDefaultMaterialLibraryChangedEventHandler(SystemEvents::OnDefaultMaterialLibraryChangedEvent::Handler& handler) { handler.Connect(m_onDefaultMaterialLibraryChangedEvent); }
        //! Register a handler to receive an event when the default SceneConfiguration changes.
        //! @param handler The handler to receive the event.
        void RegisterOnDefaultSceneConfigurationChangedEventHandler(SystemEvents::OnDefaultSceneConfigurationChangedEvent::Handler& handler) { handler.Connect(m_onDefaultSceneConfigurationChangedEvent); }

    protected:
        SystemEvents::OnInitializedEvent m_initializeEvent;
        SystemEvents::OnReinitializedEvent m_reinitializeEvent;
        SystemEvents::OnShutdownEvent m_shutdownEvent;
        SystemEvents::OnPresimulateEvent m_preSimulateEvent;
        SystemEvents::OnPostsimulateEvent m_postSimulateEvent;
        SystemEvents::OnConfigurationChangedEvent m_configChangeEvent;
        SystemEvents::OnDefaultMaterialLibraryChangedEvent m_onDefaultMaterialLibraryChangedEvent;
        SystemEvents::OnDefaultSceneConfigurationChangedEvent m_onDefaultSceneConfigurationChangedEvent;
    };
} // namespace AzPhysics
