/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <AzFramework/Physics/Common/PhysicsJoint.h>
#include <AzFramework/Physics/Configuration/JointConfiguration.h>
#include <AzFramework/Physics/Configuration/SimulatedBodyConfiguration.h>

namespace AzPhysics
{
    struct SceneConfiguration;

    //! Interface to access a Physics Scene with a SceneHandle.
    class SceneInterface
    {
    public:
        AZ_RTTI(SceneInterface, "{912CE8D1-7E3E-496F-B7BE-D17F8B30C228}");

        SceneInterface() = default;
        virtual ~SceneInterface() = default;
        AZ_DISABLE_COPY_MOVE(SceneInterface);

        //! Returns a Scene Handle connected to the given scene name.
        //! @param sceneName The name of the scene to look up.
        //! @returns Will return a SceneHandle to a Scene connected with the given name, otherwise will return InvalidSceneHandle.
        virtual SceneHandle GetSceneHandle(const AZStd::string& sceneName) = 0;

        //! Get the Scene of the requested SceneHandle.
        //! @param handle The SceneHandle of the requested scene.
        //! @return Returns a Scene pointer if found, otherwise nullptr.
        virtual Scene* GetScene(SceneHandle handle) = 0;

        //! Start the simulation process.
        //! As an example, this is a good place to trigger and queue any long running work in separate threads.
        //! @param sceneHandle The SceneHandle of the scene to use.
        //! @param deltatime The time in seconds to step the simulation for.
        virtual void StartSimulation(SceneHandle sceneHandle, float deltatime) = 0;

        //! Complete the simulation process.
        //! As an example, this is a good place to wait for any work to complete that was triggered in StartSimulation, or swap buffers if double buffering.
        //! @param sceneHandle The SceneHandle of the scene to use.
        virtual void FinishSimulation(SceneHandle sceneHandle) = 0;

        //! Enable or Disable this Scene's Simulation tick.
        //! Default is Enabled.
        //! @param sceneHandle The SceneHandle of the scene to use.
        //! @param enable When true the Scene will execute its simulation tick when StartSimulation is called. When false, StartSimulation will not execute.
        virtual void SetEnabled(SceneHandle sceneHandle, bool enable) = 0;

        //! Check if this Scene is currently Enabled.
        //! @param sceneHandle The SceneHandle of the scene to use.
        //! @return When true the Scene is enabled and will execute its simulation tick when StartSimulation is called. When false,
        //! StartSimulation will not execute or the SceneHandle is invalid.
        virtual bool IsEnabled(SceneHandle sceneHandle) const = 0;

        //! Add a simulated body to the Scene.
        //! @param sceneHandle A handle to the scene to add the requested simulated body.
        //! @param simulatedBodyConfig The config of the simulated body.
        //! @return Returns a handle to the created Simulated body. Will return AzPhyiscs::InvalidSimulatedBodyHandle if it fails.
        virtual SimulatedBodyHandle AddSimulatedBody(SceneHandle sceneHandle, const SimulatedBodyConfiguration* simulatedBodyConfig) = 0;

        //! Add a set of simulated bodied to the Scene.
        //! @param sceneHandle A handle to the scene to Add the simulated bodies to.
        //! @param simulatedBodyConfigs The list of simulated body configs.
        //! @return Returns a list of handles to the created Simulated bodies. Will be in the same order as supplied in simulatedBodyConfigs.
        //! If the scene handle is invalid, this will return an empty list. If one fails, that index will be set to AzPhyiscs::InvalidSimulatedBodyHandle.
        virtual SimulatedBodyHandleList AddSimulatedBodies(SceneHandle sceneHandle, const SimulatedBodyConfigurationList& simulatedBodyConfigs) = 0;

        //! Get the Raw pointer to the requested simulated body.
        //! @param sceneHandle A handle to the scene to get the simulated bodies from.
        //! @param bodyHandle A handle to the simulated body to retrieve the raw pointer.
        //! @return A raw pointer to the Simulated body. If the either handle is invalid this will return null.
        virtual SimulatedBody* GetSimulatedBodyFromHandle(SceneHandle sceneHandle, SimulatedBodyHandle bodyHandle) = 0;

        //! Get the Raw pointer to the set of requested simulated bodies.
        //! @param sceneHandle A handle to the scene to get the simulated bodies from.
        //! @param bodyHandles A list of simulated body handles to retrieve the raw pointers.
        //! @return A list of raw pointers to the Simulated bodies requested. If the scene handle is invalid this will return an empty list.
        //! If a simulated body handle is invalid, that index in the list will be null.
        virtual SimulatedBodyList GetSimulatedBodiesFromHandle(SceneHandle sceneHandle, const SimulatedBodyHandleList& bodyHandles) = 0;

        //! Remove a simulated body from the Scene.z
        //! @param sceneHandle A handle to the scene to remove the requested simulated body.
        //! @param bodyHandle A handle to the simulated body being removed. This will be set to AzPhysics::InvalidSimulatedBodyHandle as they're no longer valid.
        virtual void RemoveSimulatedBody(SceneHandle sceneHandle, SimulatedBodyHandle& bodyHandle) = 0;

        //! Remove a list of simulated bodies from the Scene.
        //! @param sceneHandle A handle to the scene to remove the simulated bodies from.
        //! @param bodyHandles A list of simulated body handles to be removed. All handles will be set to AzPhysics::InvalidSimulatedBodyHandle as they're no longer valid.
        virtual void RemoveSimulatedBodies(SceneHandle sceneHandle, SimulatedBodyHandleList& bodyHandles) = 0;

        //! Enable / Disable simulation of the requested body. By default all bodies added are enabled.
        //! Disabling simulation the body will no longer be affected by any forces, collisions, or found with scene queries.
        //! @param sceneHandle A handle to the scene to enable / disable the requested simulated body.
        //! @param bodyHandle The handle of the simulated body to enable / disable.
        virtual void EnableSimulationOfBody(SceneHandle sceneHandle, SimulatedBodyHandle bodyHandle) = 0;
        virtual void DisableSimulationOfBody(SceneHandle sceneHandle, SimulatedBodyHandle bodyHandle) = 0;

        //! Add a joint to the Scene.
        //! @param sceneHandle A handle to the scene to add / remove the joint.
        //! @param jointConfig The config of the joint.
        //! @param parentBody The parent body of the joint.
        //! @param childBody The child body of the joint
        //! @return Returns a handle to the created joint. Will return AzPhyiscs::InvalidJointHandle if it fails.
        virtual JointHandle AddJoint(SceneHandle sceneHandle, const JointConfiguration* jointConfig, 
            SimulatedBodyHandle parentBody, SimulatedBodyHandle childBody) = 0;

        //! Get the Raw pointer to the requested joint.
        //! @param sceneHandle A handle to the scene to get the simulated bodies from.
        //! @param jointHandle A handle to the joint to retrieve the raw pointer.
        //! @return A raw pointer to the Joint body. If the either handle is invalid this will return null.
        virtual Joint* GetJointFromHandle(SceneHandle sceneHandle, JointHandle jointHandle) = 0;

        //! Remove a joint from the Scene.
        //! @param sceneHandle A handle to the scene to add / remove the joint.
        //! @param jointHandle A handle to the joint being removed.
        virtual void RemoveJoint(SceneHandle sceneHandle, JointHandle jointHandle) = 0;

        //! Make a blocking query into the scene.
        //! @param sceneHandle A handle to the scene to make the scene query with.
        //! @param request The request to make. Should be one of RayCastRequest || ShapeCastRequest || OverlapRequest
        //! @return Returns a structure that contains a list of Hits. Depending on flags set in the request, this may only contain 1 result.
        virtual SceneQueryHits QueryScene(SceneHandle sceneHandle, const SceneQueryRequest* request) = 0;

        //! Make a blocking query into the scene.
        //! @param sceneHandle A handle to the scene to make the scene query with.
        //! @param request The request to make. Should be one of RayCastRequest || ShapeCastRequest || OverlapRequest
        //! @param result A structure that contains a list of Hits. Depending on flags set in the request, this may only contain 1 result.
        //! @return Returns true if there is at least one hit.
        virtual bool QueryScene(SceneHandle sceneHandle, const SceneQueryRequest* request, SceneQueryHits& result) = 0;

        //! Make many blocking queries into the scene.
        //! @param sceneHandle A handle to the scene to make the scene query with.
        //! @param requests A list of requests to make. Each entry should be one of RayCastRequest || ShapeCastRequest || OverlapRequest
        //! @return Returns a list of SceneQueryHits. Will be in the same order as supplied in SceneQueryRequests.
        virtual SceneQueryHitsList QuerySceneBatch(SceneHandle sceneHandle, const SceneQueryRequests& requests) = 0;

        //! Make a non-blocking query into the scene.
        //! @param sceneHandle A handle to the scene to make the scene query with.
        //! @param requestId A user defined value to identify the request when the callback is called.
        //! @param request The request to make. Should be one of RayCastRequest || ShapeCastRequest || OverlapRequest
        //! @param callback The callback to trigger when the request is complete.
        //! @return Returns If the request was queued successfully. If returns false, the callback will never be called.
        [[nodiscard]] virtual bool QuerySceneAsync(SceneHandle sceneHandle, SceneQuery::AsyncRequestId requestId,
            const SceneQueryRequest* request, SceneQuery::AsyncCallback callback) = 0;

        //! Make a non-blocking query into the scene.
        //! @param sceneHandle A handle to the scene to make the scene query with.
        //! @param requestId A user defined valid to identify the request when the callback is called.
        //! @param requests A list of requests to make. Each entry should be one of RayCastRequest || ShapeCastRequest || OverlapRequest
        //! @param callback The callback to trigger when all the request are complete.
        //! @return Returns If the request was queued successfully. If returns false, the callback will never be called.
        [[nodiscard]] virtual bool QuerySceneAsyncBatch(SceneHandle sceneHandle, SceneQuery::AsyncRequestId requestId,
            const SceneQueryRequests& requests, SceneQuery::AsyncBatchCallback callback) = 0;

        //! Registers a pair of simulated bodies for which collisions should be suppressed.
        //! Making multiple requests with the same pair result are dropped. To remove the suppression call UnsuppressCollisionEvents with the pair.
        //! The order of the bodies do not matter, {body0, body1} collision pair is equal to {body1, body0}.
        //! @param sceneHandle A handle to the scene to register the collision pair with.
        //! @param bodyHandleA A handle to a simulated body.
        //! @param bodyHandleB A handle to a simulated body.
        virtual void SuppressCollisionEvents(SceneHandle sceneHandle,
            const SimulatedBodyHandle& bodyHandleA,
            const SimulatedBodyHandle& bodyHandleB) = 0;

        //! Unregisters a pair of simulated bodies for which collisions should be suppressed.
        //! Making multiple requests with the same pair result are dropped. To add a suppression call SuppressCollisionEvents with the pair.
        //! The order of the bodies do not matter, {body0, body1} collision pair is equal to {body1, body0}.
        //! @param sceneHandle A handle to the scene to unregister the collision pair with.
        //! @param bodyHandleA A handle to a simulated body.
        //! @param bodyHandleB A handle to a simulated body.
        virtual void UnsuppressCollisionEvents(SceneHandle sceneHandle,
            const SimulatedBodyHandle& bodyHandleA,
            const SimulatedBodyHandle& bodyHandleB) = 0;

        //! Set the Gravity of the given Scene.
        //! @param sceneHandle A handle to the scene to set the gravity vector of.
        //! @Param The new gravity vector to be used in the Scene
        virtual void SetGravity(SceneHandle sceneHandle, const AZ::Vector3& gravity) = 0;

        //! Get the Gravity of the given Scene.
        //! @param sceneHandle A handle to the scene to get the gravity vector of.
        //! @return A Vector3 of the gravity used in the Scene, will return a Zero Vector if sceneHandle is invalid or not found.
        virtual AZ::Vector3 GetGravity(SceneHandle sceneHandle) const = 0;

        //! Register a handler to receive an event when the SceneConfiguration changes.
        //! @param sceneHandle A handle to the scene to register the event with.
        //! @param handler The handler to receive the event.
        virtual void RegisterSceneConfigurationChangedEventHandler(SceneHandle sceneHandle, SceneEvents::OnSceneConfigurationChanged::Handler& handler) = 0;

        //! Register a handler to receive an event when a Simulated body is added to the Scene.
        //! @param sceneHandle A handle to the scene to register the event with.
        //! @param handler The handler to receive the event.
        virtual void RegisterSimulationBodyAddedHandler(SceneHandle sceneHandle, SceneEvents::OnSimulationBodyAdded::Handler& handler) = 0;

        //! Register a handler to receive an event when a Simulated body is removed from the Scene.
        //! @param sceneHandle A handle to the scene to register the event with.
        //! @param handler The handler to receive the event.
        virtual void RegisterSimulationBodyRemovedHandler(SceneHandle sceneHandle, SceneEvents::OnSimulationBodyRemoved::Handler& handler) = 0;

        //! Register a handler to receive an event when a Simulated body has its simulation enabled in the Scene.
        //! @param sceneHandle A handle to the scene to register the event with.
        //! This will only trigger if the simulated body was disabled, when first added to a scene SceneEvents::OnAnySimulationBodyCreated will trigger instead.
        //! @param handler The handler to receive the event.
        virtual void RegisterSimulationBodySimulationEnabledHandler(SceneHandle sceneHandle, SceneEvents::OnSimulationBodySimulationEnabled::Handler& handler) = 0;

        //! Register a handler to receive an event when a Simulated body has its simulation disabled in the Scene.
        //! @param sceneHandle A handle to the scene to register the event with.
        //! @param handler The handler to receive the event.
        virtual void RegisterSimulationBodySimulationDisabledHandler(SceneHandle sceneHandle, SceneEvents::OnSimulationBodySimulationDisabled::Handler& handler) = 0;

        //! Register a handler to receive an event when Scene::StartSimulation is called.
        //! @note This may fire multiple times per frame.
        //! @param sceneHandle A handle to the scene to register the event with.
        //! @param handler The handler to receive the event.
        virtual void RegisterSceneSimulationStartHandler(SceneHandle sceneHandle, SceneEvents::OnSceneSimulationStartHandler& handler) = 0;

        //! Register a handler to receive an event when Scene::FinishSimulation is called.
        //! @note This may fire multiple times per frame.
        //! @param sceneHandle A handle to the scene to register the event with.
        //! @param handler The handler to receive the event.
        virtual void RegisterSceneSimulationFinishHandler(SceneHandle sceneHandle, SceneEvents::OnSceneSimulationFinishHandler& handler) = 0;

        //! Register a handler to receive an event with a list of SimulatedBodyHandles that updated this scene tick.
        //! @note This will fire after the OnSceneSimulationStartEvent and before the OnSceneSimulationFinishEvent when SceneConfiguration::m_enableActiveActors is true.
        //! @param sceneHandle A handle to the scene to register the event with.
        //! @param handler The handler to receive the event.
        virtual void RegisterSceneActiveSimulatedBodiesHandler(SceneHandle sceneHandle, SceneEvents::OnSceneActiveSimulatedBodiesEvent::Handler& handler) = 0;

        //! Register a handler to receive all collision events in the scene.
        //! @param sceneHandle A handle to the scene to register the event with.
        //! @param handler The handler to receive the event.
        virtual void RegisterSceneCollisionEventHandler(SceneHandle sceneHandle, SceneEvents::OnSceneCollisionsEvent::Handler& handler) = 0;

        //! Register a handler to receive all trigger events in the scene.
        //! @param sceneHandle A handle to the scene to register the event with.
        //! @param handler The handler to receive the event.
        virtual void RegisterSceneTriggersEventHandler(SceneHandle sceneHandle, SceneEvents::OnSceneTriggersEvent::Handler& handler) = 0;

        //! Register a handler to receive a notification when the Scene's gravity has changed.
        //! @param sceneHandle A handle to the scene to register the event with.
        //! @param handler The handler to receive the event.
        virtual void RegisterSceneGravityChangedEvent(SceneHandle sceneHandle, SceneEvents::OnSceneGravityChangedEvent::Handler& handler) = 0;
    };

    //! Interface of a Physics Scene
    class Scene
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(Scene, "{52BD8163-BDC4-4B09-ABB2-11DD1F601FFD}");
        static void Reflect(AZ::ReflectContext* context);

        explicit Scene(const SceneConfiguration& config);
        virtual ~Scene() = default;

        //! Get the Id of the Scene.
        //! @return The Crc32 of the scene.
        const AZ::Crc32& GetId() const;

        //! Start the simulation process.
        //! As an example, this is a good place to trigger and queue any long running work in separate threads.
        //! @param deltatime The time in seconds to run the simulation for.
        virtual void StartSimulation(float deltatime) = 0;

        //! Complete the simulation process.
        //! As an example, this is a good place to wait for any work to complete that was triggered in StartSimulation, or swap buffers if double buffering.
        virtual void FinishSimulation() = 0;

        //! Enable or Disable this Scene's Simulation tick.
        //! Default is Enabled.
        //! @param enable When true the Scene will execute its simulation tick when StartSimulation is called. When false, StartSimulation will not execute.
        virtual void SetEnabled(bool enable) = 0;

        //! Check if this Scene is currently Enabled.
        //! @return When true the Scene is enabled and will execute its simulation tick when StartSimulation is called. When false, StartSimulation will not execute.
        virtual bool IsEnabled() const = 0;

        //! Accessor to the Scenes Configuration.
        //! @returns Return the currently used SceneConfiguration.
        virtual const SceneConfiguration& GetConfiguration() const = 0;

        //! Update the SceneConfiguration.
        //! @param config The new configuration to apply.
        virtual void UpdateConfiguration(const SceneConfiguration& config) = 0;

        //! Add a simulated body to the Scene.
        //! @param simulatedBodyConfig The config of the simulated body.
        //! @return Returns a handle to the created Simulated body. Will return AzPhyiscs::InvalidSimulatedBodyHandle if it fails.
        virtual SimulatedBodyHandle AddSimulatedBody(const SimulatedBodyConfiguration* simulatedBodyConfig) = 0;

        //! Add a set of simulated bodied to the Scene.
        //! @param simulatedBodyConfigs The list of simulated body configs.
        //! @return Returns a list of handles to the created Simulated bodies. Will be in the same order as supplied in simulatedBodyConfigs.
        //! If one fails, that index will be set to AzPhyiscs::InvalidSimulatedBodyHandle.
        virtual SimulatedBodyHandleList AddSimulatedBodies(const SimulatedBodyConfigurationList& simulatedBodyConfigs) = 0;

        //! Get the Raw pointer to the requested simulated body.
        //! @param bodyHandle A handle to the simulated body to retrieve the raw pointer for.
        //! @return A raw pointer to the Simulated body. If the handle is invalid this will return null.
        virtual SimulatedBody* GetSimulatedBodyFromHandle(SimulatedBodyHandle bodyHandle) = 0;

        //! Get the Raw pointer to the set of requested simulated bodies.
        //! @param bodyHandles A list of simulated body handles to retrieve the raw pointers for.
        //! @return A list of raw pointers to the Simulated bodies requested. If a simulated body handle is invalid, that index in the list will be null.
        virtual SimulatedBodyList GetSimulatedBodiesFromHandle(const SimulatedBodyHandleList& bodyHandles) = 0;

        //! Remove a simulated body from the Scene.
        //! @param bodyHandle A handle to the simulated body being removed. This will be set to AzPhysics::InvalidSimulatedBodyHandle as they're no longer valid.
        virtual void RemoveSimulatedBody(SimulatedBodyHandle& bodyHandle) = 0;

        //! Remove a list of simulated bodies from the Scene.
        //! @param bodyHandles A list of simulated body handles to be removed. All handles will be set to AzPhysics::InvalidSimulatedBodyHandle as they're no longer valid.
        virtual void RemoveSimulatedBodies(SimulatedBodyHandleList& bodyHandles) = 0;

        //! Enable / Disable simulation of the requested body. By default all bodies added are enabled.
        //! Disabling simulation the body will no longer be affected by any forces, collisions, or found with scene queries.
        //! @param bodyHandle The handle of the simulated body to enable / disable.
        virtual void EnableSimulationOfBody(SimulatedBodyHandle bodyHandle) = 0;
        virtual void DisableSimulationOfBody(SimulatedBodyHandle bodyHandle) = 0;

        //! Add a joint to the Scene.
        //! @param jointConfig The config of the joint.
        //! @param parentBody The parent body of the joint.
        //! @param childBody The child body of the joint
        //! @return Returns a handle to the created joint. Will return AzPhyiscs::InvalidJointHandle if it fails.
        virtual JointHandle AddJoint(const JointConfiguration* jointConfig, 
            SimulatedBodyHandle parentBody, SimulatedBodyHandle childBody) = 0;

        //! Get the Raw pointer to the requested joint.
        //! @param jointHandle A handle to the joint to retrieve the raw pointer.
        //! @return A raw pointer to the Joint body. If the either handle is invalid this will return null.
        virtual Joint* GetJointFromHandle(JointHandle jointHandle) = 0;

        //! Remove a joint from the Scene.
        //! @param jointHandle A handle to the joint being removed.
        virtual void RemoveJoint(JointHandle jointHandle) = 0;

        //! Make a blocking query into the scene.
        //! @param request The request to make. Should be one of RayCastRequest || ShapeCastRequest || OverlapRequest
        //! @return Returns a structure that contains a list of Hits. Depending on flags set in the request, this may only contain 1 result.
        virtual SceneQueryHits QueryScene(const SceneQueryRequest* request) = 0;

        //! Make a blocking query into the scene.
        //! @param request The request to make. Should be one of RayCastRequest || ShapeCastRequest || OverlapRequest
        //! @param result A structure that contains a list of Hits. Depending on flags set in the request, this may only contain 1 result.
        //! @return Returns true if there is at least one hit.
        virtual bool QueryScene(const SceneQueryRequest* request, SceneQueryHits& result) = 0;

        //! Make many blocking queries into the scene.
        //! @param requests A list of requests to make. Each entry should be one of RayCastRequest || ShapeCastRequest || OverlapRequest
        //! @return Returns a list of SceneQueryHits. Will be in the same order as supplied in SceneQueryRequests.
        virtual SceneQueryHitsList QuerySceneBatch(const SceneQueryRequests& requests) = 0;

        //! Make a non-blocking query into the scene.
        //! @param requestId A user defined valid to identify the request when the callback is called.
        //! @param request The request to make. Should be one of RayCastRequest || ShapeCastRequest || OverlapRequest
        //! @param callback The callback to trigger when the request is complete.
        //! @return Returns if the request was queued successfully. If returns false, the callback will never be called.
        [[nodiscard]] virtual bool QuerySceneAsync(SceneQuery::AsyncRequestId requestId,
            const SceneQueryRequest* request, SceneQuery::AsyncCallback callback) = 0;

        //! Make a non-blocking query into the scene.
        //! @param requestId A user defined valid to identify the request when the callback is called.
        //! @param requests A list of requests to make. Each entry should be one of RayCastRequest || ShapeCastRequest || OverlapRequest
        //! @param callback The callback to trigger when all the request are complete.
        //! @return Returns If the request was queued successfully. If returns false, the callback will never be called.
        [[nodiscard]] virtual bool QuerySceneAsyncBatch(SceneQuery::AsyncRequestId requestId,
            const SceneQueryRequests& requests, SceneQuery::AsyncBatchCallback callback) = 0;

        //! Registers a pair of simulated bodies for which collisions should be suppressed.
        //! Making multiple requests with the same pair result are dropped. To remove the suppression call UnsuppressCollisionEvents with the pair.
        //! The order of the bodies do not matter, {body0, body1} collision pair is equal to {body1, body0}.
        //! @param bodyHandleA A handle to a simulated body.
        //! @param bodyHandleB A handle to a simulated body.
        virtual void SuppressCollisionEvents(
            const SimulatedBodyHandle& bodyHandleA,
            const SimulatedBodyHandle& bodyHandleB) = 0;

        //! Unregisters a pair of simulated bodies for which collisions should be suppressed.
        //! Making multiple requests with the same pair result are dropped. To add a suppression call SuppressCollisionEvents with the pair.
        //! The order of the bodies do not matter, {body0, body1} collision pair is equal to {body1, body0}.
        //! @param bodyHandleA A handle to a simulated body.
        //! @param bodyHandleB A handle to a simulated body.
        virtual void UnsuppressCollisionEvents(
            const SimulatedBodyHandle& bodyHandleA,
            const SimulatedBodyHandle& bodyHandleB) = 0;

        //! Set the Gravity of the Scene.
        //! @Param The new gravity vector to be used in the Scene
        virtual void SetGravity(const AZ::Vector3& gravity) = 0;

        //! Get the Gravity of the Scene.
        //! @return A Vector3 of the gravity used in the Scene.
        virtual AZ::Vector3 GetGravity() const = 0;

        //! Get the native pointer for a scene. Should be used with caution as it allows direct access to the lower level physics simulation.
        //! @return A pointer to the underlying implementation of a scene if there is one.
        virtual void* GetNativePointer() const = 0;

        //! Register a handler to receive an event when the SceneConfiguration changes.
        //! @param handler The handler to receive the event.
        void RegisterSceneConfigurationChangedEventHandler(SceneEvents::OnSceneConfigurationChanged::Handler& handler);

        //! Register a handler to receive an event when a Simulated body is added to the Scene.
        //! @param handler The handler to receive the event.
        void RegisterSimulationBodyAddedHandler(SceneEvents::OnSimulationBodyAdded::Handler& handler);

        //! Register a handler to receive an event when a Simulated body is removed from the Scene.
        //! @param handler The handler to receive the event.
        void RegisterSimulationBodyRemovedHandler(SceneEvents::OnSimulationBodyRemoved::Handler& handler);

        //! Register a handler to receive an event when a Simulated body has its simulation enabled in the Scene.
        //! This will only trigger if the simulated body was disabled, when first added to a scene SceneEvents::OnAnySimulationBodyCreated will trigger instead.
        //! @param handler The handler to receive the event.
        void RegisterSimulationBodySimulationEnabledHandler(SceneEvents::OnSimulationBodySimulationEnabled::Handler& handler);

        //! Register a handler to receive an event when a Simulated body has its simulation disabled in the Scene.
        //! @param handler The handler to receive the event.
        void RegisterSimulationBodySimulationDisabledHandler(SceneEvents::OnSimulationBodySimulationDisabled::Handler& handler);

        //! Register a handler to receive an event when Scene::StartSimulation is called.
        //! @note This may fire multiple times per frame.
        //! @param handler The handler to receive the event.
        void RegisterSceneSimulationStartHandler(SceneEvents::OnSceneSimulationStartHandler& handler);

        //! Register a handler to receive an event when Scene::FinishSimulation is called.
        //! @note This may fire multiple times per frame.
        //! @param handler The handler to receive the event.
        void RegisterSceneSimulationFinishHandler(SceneEvents::OnSceneSimulationFinishHandler& handler);

        //! Register a handler to receive an event with a list of SimulatedBodyHandles that updated this scene tick.
        //! @note This will fire after the OnSceneSimulationStartEvent and before the OnSceneSimulationFinishEvent when SceneConfiguration::m_enableActiveActors is true.
        //! @param handler The handler to receive the event.
        void RegisterSceneActiveSimulatedBodiesHandler(SceneEvents::OnSceneActiveSimulatedBodiesEvent::Handler& handler);

        //! Register a handler to receive all collision events in the scene.
        //! @param handler The handler to receive the event.
        void RegisterSceneCollisionEventHandler(SceneEvents::OnSceneCollisionsEvent::Handler& handler);

        //! Register a handler to receive all trigger events in the scene.
        //! @param handler The handler to receive the event.
        void RegisterSceneTriggersEventHandler(SceneEvents::OnSceneTriggersEvent::Handler& handler);

        //! Register a handler to receive a notification when the Scene's gravity has changed.
        //! @param handler The handler to receive the event.
        void RegisterSceneGravityChangedEvent(SceneEvents::OnSceneGravityChangedEvent::Handler& handler);

    protected:
        SceneEvents::OnSceneConfigurationChanged m_configChangeEvent;
        SceneEvents::OnSimulationBodyAdded m_simulatedBodyAddedEvent;
        SceneEvents::OnSimulationBodyRemoved m_simulatedBodyRemovedEvent;
        SceneEvents::OnSimulationBodySimulationEnabled m_simulatedBodySimulationEnabledEvent;
        SceneEvents::OnSimulationBodySimulationDisabled m_simulatedBodySimulationDisabledEvent;
        SceneEvents::OnSceneSimulationStartEvent m_sceneSimulationStartEvent;
        SceneEvents::OnSceneSimulationFinishEvent m_sceneSimulationFinishEvent;
        SceneEvents::OnSceneActiveSimulatedBodiesEvent m_sceneActiveSimulatedBodies;
        SceneEvents::OnSceneCollisionsEvent m_sceneCollisionEvent;
        SceneEvents::OnSceneTriggersEvent m_sceneTriggerEvent;
        SceneEvents::OnSceneGravityChangedEvent m_sceneGravityChangedEvent;
    private:
        // helper for behaviour context
        SceneEvents::OnSceneGravityChangedEvent* GetOnGravityChangedEvent();

        AZ::Crc32 m_id;
    };
    using SceneList = AZStd::vector<AZStd::unique_ptr<Scene>>;

    inline void Scene::RegisterSceneConfigurationChangedEventHandler(SceneEvents::OnSceneConfigurationChanged::Handler& handler)
    {
        handler.Connect(m_configChangeEvent);
    }

    inline void Scene::RegisterSimulationBodyAddedHandler(SceneEvents::OnSimulationBodyAdded::Handler& handler)
    {
        handler.Connect(m_simulatedBodyAddedEvent);
    }

    inline void Scene::RegisterSimulationBodyRemovedHandler(SceneEvents::OnSimulationBodyRemoved::Handler& handler)
    {
        handler.Connect(m_simulatedBodyRemovedEvent);
    }

    inline void Scene::RegisterSimulationBodySimulationEnabledHandler(SceneEvents::OnSimulationBodySimulationEnabled::Handler& handler)
    {
        handler.Connect(m_simulatedBodySimulationEnabledEvent);
    }

    inline void Scene::RegisterSimulationBodySimulationDisabledHandler(SceneEvents::OnSimulationBodySimulationDisabled::Handler& handler)
    {
        handler.Connect(m_simulatedBodySimulationDisabledEvent);
    }

    inline void Scene::RegisterSceneSimulationStartHandler(SceneEvents::OnSceneSimulationStartHandler& handler)
    {
        handler.Connect(m_sceneSimulationStartEvent);
    }

    inline void Scene::RegisterSceneSimulationFinishHandler(SceneEvents::OnSceneSimulationFinishHandler& handler)
    {
        handler.Connect(m_sceneSimulationFinishEvent);
    }

    inline void Scene::RegisterSceneActiveSimulatedBodiesHandler(SceneEvents::OnSceneActiveSimulatedBodiesEvent::Handler& handler)
    {
        handler.Connect(m_sceneActiveSimulatedBodies);
    }

    inline void Scene::RegisterSceneCollisionEventHandler(SceneEvents::OnSceneCollisionsEvent::Handler& handler)
    {
        handler.Connect(m_sceneCollisionEvent);
    }

    inline void Scene::RegisterSceneTriggersEventHandler(SceneEvents::OnSceneTriggersEvent::Handler& handler)
    {
        handler.Connect(m_sceneTriggerEvent);
    }

    inline void Scene::RegisterSceneGravityChangedEvent(SceneEvents::OnSceneGravityChangedEvent::Handler& handler)
    {
        handler.Connect(m_sceneGravityChangedEvent);
    }
} // namespace AzPhysics
