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

#include <AzCore/EBus/EBus.h>

#include <I3DEngine.h>

namespace LmbrCentral
{
    //Forward decl
    enum class StandinType : AZ::u32;

    class EditorGeometryCacheComponentRequests
        : public AZ::ComponentBus
    {
    public:
        // EBus Traits overrides (Configuring this EBus)
        // Using Defaults

        virtual ~EditorGeometryCacheComponentRequests() {}

        /**
         * Sets the minimum spec for the Geometry Cache
         *
         * The geom cache will not render when the current graphics spec is 
         * below this minimum spec. 
         */
        virtual void SetMinSpec(EngineSpec minSpec) = 0;
        //! Gets the minimum spec for the Geom Cache
        virtual EngineSpec GetMinSpec() = 0;

        /**
         * Sets whether or not the Geom Cache's animation will play on start
         *
         * When set to true the Geometry Cache will play in the editor. In game
         * mode the Geom Cache will begin playing immediately. 
         */
        virtual void SetPlayOnStart(bool playOnStart) = 0;
        //! Gets whether or not the Geom Cache's animation will play on start
        virtual bool GetPlayOnStart() = 0;

        /**
         * Sets the max view distance that this Geom Cache will be visible from
         *
         * The actual max view distance is calculated from 
         * maxViewDistance * viewDistanceMultiplier. 
         */
        virtual void SetMaxViewDistance(float maxViewDistance) = 0;
        /**
         * Gets the max view distance that the Geom Cache will be visible from
         *
         * This is the max view distance without the view distance multiplier
         */
        virtual float GetMaxViewDistance() = 0;

        /**
         * Sets the view distance multiplier 
         *
         * The view distance multiplier is multiplied into the max view distance
         * to determine how far you can be from the Geom Cache before it stops
         * being rendered.
         *
         * A value of 1.0 will leave the max view distance as the actual max view distance.
         */
        virtual void SetViewDistanceMultiplier(float viewDistanceMultiplier) = 0;
        //! Gets the view distance multiplier
        virtual float GetViewDistanceMultiplier() = 0;

        /**
         * Sets the LOD Distance Ratio
         *
         * The LOD Distance ratio affects how LODs are chosen. A lower value means
         * less detailed LODs are used at shorter view distances.
         */
        virtual void SetLODDistanceRatio(AZ::u32 lodDistanceRatio) = 0;
        //! Gets the LOD Distance Ratio
        virtual AZ::u32 GetLODDistanceRatio() = 0;

        //! Sets whether or not the Geometry Cache will cast shadows
        virtual void SetCastShadows(bool castShadows) = 0;
        //! Gets whether or not the Geometry Cache casts shadows
        virtual bool GetCastShadows() = 0;

        //! Sets whether or not the Geometry Cache will be affected by VisAreas and Portals
        virtual void SetUseVisAreas(bool useVisAreas) = 0;
        //! Gets whether or not the Geometry Cache is affected by VisAreas and Portals
        virtual bool GetUseVisAreas() = 0;
    };

    using EditorGeometryCacheComponentRequestBus = AZ::EBus<EditorGeometryCacheComponentRequests>;

    class GeometryCacheComponentRequests
        : public AZ::ComponentBus
    {
    public:
        // EBus Traits overrides (Configuring this EBus)
        // Using Defaults

        virtual ~GeometryCacheComponentRequests() {}

        /**
         * Begins Geometry Cache animation playback
         * 
         * If the animation is already playing, this does nothing.
         * This may be the case if PlayOnStart was set during edit mode.
         */
        virtual void Play() = 0;
        //! Pauses Geometry Cache animation playback
        virtual void Pause() = 0;
        /**
         * Stops Geometry Cache animation playback
         *
         * This will reset the playback time to 0. This means
         * that when Play is called again that the animation 
         * will start from StartTime.
         */
        virtual void Stop() = 0;

        /**
         * Gets how much time remains in the animation
         *
         * Returns -1.0 if the animation is paused or stopped.
         *
         * @return How much time in seconds is left in the animation
         */
        virtual float GetTimeRemaining() = 0;

        /**
         * Gets the type of the stand-in that's currently in use
         *
         * If no stand-in is in use this will return 
         * StandinType::None.
         *
         * @return The type of stand-in currently in use
         */
        virtual StandinType GetCurrentStandinType() = 0;

        /**
         * Sets the geom cache asset to be rendered
         */
        virtual void SetGeomCacheAsset(const AZ::Data::AssetId& id) = 0;

        /**
         * Gets the geom cache asset in use by this component
         */
        virtual AZ::Data::Asset<AZ::Data::AssetData> GetGeomCacheAsset() = 0;

        /**
         * Sets whether or not the Geometry Cache will be processed for rendering
         *
         * If Visibility is turned off, all Stand-ins for this 
         * Geometry Cache will also be turned off.
         *
         * This does not mean that the Geometry Cache is visible in the current
         * frame or the next frame. This just means that it will be 
         * submitted to the visibility system for rendering. It can still
         * be culled.
         *
         * @param visible Pass True for the Geometry Cache to render
         */
        virtual void SetVisible(bool visible) = 0;
        /**
         * Gets whether or not the Geometry Cache will be processed for rendering
         *
         * If this returns false then all related Stand-ins should also
         * not be visible.
         *
         * @return True if the Geometry Cache should be rendered.
         */
        virtual bool GetVisible() = 0;

        /**
         * Sets whether or not the Geometry Cache animation should loop
         *
         * The Last Frame Stand-in will never be visible as long as this remains
         * true. 
         *
         * @param loop Pass true if you want the Geometry Cache animation to loop
         */
        virtual void SetLoop(bool loop) = 0;
        /**
         * Gets whether or not the Geometry Cache animation is set to loop
         *
         * @return True if the Geometry Cache animation will loop
         */
        virtual bool GetLoop() = 0;

        /**
         * Sets the time point that the Geometry Cache animation should start at
         *
         * Changing the start time of the animation will restart the animation. 
         * 
         * @param startTime The starting time point in seconds.
         */
        virtual void SetStartTime(float startTime) = 0;
        /**
         * Gets the current start time point for the Geometry Cache animation
         *
         * @return The Geometry Cache's animation start time in seconds.
         */
        virtual float GetStartTime() = 0;

        /**
        * Sets the distance threshold that controls Geometry Cache streaming
        *
        * When the distance between the center of the Geometry Cache and the
        * current camera is greater than this value the Geometry Cache's animation
        * will begin to stream into memory.
        *
        * @param streamInDistance The distance threshold for Geometry Cache streaming
        */
        virtual void SetStreamInDistance(float streamInDistance) = 0;
        /**
        * Gets the distance threshold that controls when the Geometry Cache will stream to memory
        *
        * @return The distance threshold for Geometry Cache streaming
        */
        virtual float GetStreamInDistance() = 0;

        /**
         * Sets the entity to be used for the First Frame Stand-in
         *
         * It's assumed that the entityId points to an entity that has
         * a MeshComponent attached. The stand-in is controlled by the
         * visibility parameter of the MeshComponent.
         *
         * The given entity will not be transformed or moved at all. The
         * only change that will be made to it will be when it's made
         * visible/invisible as a stand-in.
         *
         * Invalid entity ids will be ignored.
         *
         * This stand-in will be used until the Geometry Cache animation 
         * starts playing.
         *
         * @param entityId The ID of the Entity to use as the First Frame Stand-in
         */
        virtual void SetFirstFrameStandIn(AZ::EntityId entityId) = 0;
        /**
         * Gets the entity that is used as the First Frame Stand-in
         *
         * @return The entity with a mesh component that's used as the First Frame Stand-in
         */
        virtual AZ::EntityId GetFirstFrameStandIn() = 0;

        /**
         * Sets the entity to be used for the Last Frame Stand-in
         *
         * It's assumed that the entityId points to an entity that has
         * a MeshComponent attached. The stand-in is controlled by the
         * visibility parameter of the MeshComponent.
         *
         * The given entity will not be transformed or moved at all. The
         * only change that will be made to it will be when it's made
         * visible/invisible as a stand-in.
         *
         * Invalid entity ids will be ignored.
         *
         * This stand-in will never be used if the Loop parameter is set to true.
         *
         * @param entityId The ID of the Entity to use as the Last Frame Stand-in
         */
        virtual void SetLastFrameStandIn(AZ::EntityId entityId) = 0;
        /**
         * Gets the entity that is used as the Last Frame Stand-in
         *
         * @return The entity with a mesh component that's used as the Last Frame Stand-in
         */
        virtual AZ::EntityId GetLastFrameStandIn() = 0;

        /**
         * Sets the entity to be used for the distance-based Stand-in
         *
         * It's assumed that the entityId points to an entity that has
         * a MeshComponent attached. The stand-in is controlled by the
         * visibility parameter of the MeshComponent.
         *
         * The given entity will not be transformed or moved at all. The
         * only change that will be made to it will be when it's made
         * visible/invisible as a stand-in.
         *
         * Invalid entity ids will be ignored.
         *
         * This Stand-in will be used as long as the distance between the 
         * Geometry Cache's center and the current camera's position is larger
         * than the Stand-in Distance parameter.
         *
         * @param entityId The ID of the Entity to use as the distance-based Stand-in
         */
        virtual void SetStandIn(AZ::EntityId entityId) = 0;
        /**
         * Gets the entity that is used as the distance-based Stand-in
         *
         * @return The entity with a mesh component that's used as the distance-based Stand-in
         */
        virtual AZ::EntityId GetStandIn() = 0;

        /**
         * Sets the distance threshold that controls the visibility of the stand-in
         *
         * The stand-in will be used when the distance between the center of the
         * Geometry Cache and the current camera is greater than this value.
         * 
         * @param standInDistance The distance threshold for the stand-in
         */
        virtual void SetStandInDistance(float standInDistance) = 0;
        /**
         * Gets the distance threshold that controls the visibility of the stand-in
         *
         * @return The distance threshold for the stand-in
         */
        virtual float GetStandInDistance() = 0;

        /**
         * Gets the Geometry Cache's render node
         *
         * This method is only exposed to C++.
         *
         * @return A pointer to the geometry cache's internal render node
         */
        virtual IGeomCacheRenderNode* GetGeomCacheRenderNode() = 0;
    };

    using GeometryCacheComponentRequestBus = AZ::EBus<GeometryCacheComponentRequests>;

    class GeometryCacheComponentNotifications
        : public AZ::EBusTraits
    {
    public:
        // EBus Traits overrides (Configuring this EBus)
        // Using Defaults

        virtual ~GeometryCacheComponentNotifications() {}

        //! Event that triggers when Geometry Cache playback starts
        virtual void OnPlaybackStart() = 0;
        //! Event that triggers when Geometry Cache playback pauses
        virtual void OnPlaybackPause() = 0;
        //! Event that triggers when Geometry Cache playback stops
        virtual void OnPlaybackStop() = 0;
        
        /**
         * Event that triggers when the Geometry Cache changes which stand-in is in use
         *
         * This does fire if a stand-in is turned off and the Geometry Cache becomes 
         * active instead.
         *
         * @param standinType The new stand-in type that will be used
         */
        virtual void OnStandinChanged(StandinType standinType) = 0;
    };

    using GeometryCacheComponentNotificationBus = AZ::EBus<GeometryCacheComponentNotifications>;

} //namespace LmbrCentral
