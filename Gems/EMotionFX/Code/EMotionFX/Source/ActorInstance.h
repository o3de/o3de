/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Color.h>
#include <AzCore/RTTI/TypeInfo.h>
#include "EMotionFXConfig.h"
#include <MCore/Source/Vector.h>
#include <MCore/Source/Ray.h>
#include <MCore/Source/MultiThreadManager.h>
#include "Actor.h"
#include "Transform.h"
#include "AnimGraphPosePool.h"

#include <Atom/RPI.Reflect/Model/ModelAsset.h>


namespace Physics
{
    class Ragdoll;
}

namespace EMotionFX
{
    class MotionSystem;
    class Attachment;
    class AnimGraphInstance;
    class MorphSetupInstance;
    class RagdollInstance;


    /**
     * The actor instance class.
     * An actor instance represents an animated character in your game.
     * Each actor instance is created from some Actor object, which contains all the hierarchy information and possibly also
     * the transformation and mesh information. Still, each actor instance can be controlled and animated individually.
     */
    class EMFX_API ActorInstance
        : public MCore::RefCounted
    {
        AZ_CLASS_ALLOCATOR_DECL
        friend class Attachment;

    public:
        AZ_RTTI(EMotionFX::ActorInstance, "{280A0170-EB6A-4E90-B2F1-E18D8EAEFB36}");
        /**
         * The bounding volume generation types.
         */
        enum EBoundsType : AZ::u8
        {
            BOUNDS_NODE_BASED           = 0,    /**< Calculate the bounding volumes based on the world space node positions. */
            BOUNDS_MESH_BASED           = 1,    /**< Calculate the bounding volumes based on the world space vertex positions. */
            BOUNDS_STATIC_BASED         = 5     /**< Calculate the bounding volumes based on an approximate box, based on the mesh bounds, and move this box along with the actor instance position. */
        };

        static ActorInstance* Create(Actor* actor, AZ::Entity* entity = nullptr, uint32 threadIndex = 0);

        /**
         * Get a pointer to the actor from which this is an instance.
         * @result A pointer to the actor from which this is an instance.
         */
        Actor* GetActor() const;

        /**
         * Get the unique identification number for the actor instance.
         * @return The unique identification number.
         */
        MCORE_INLINE uint32 GetID() const                                       { return m_id; }

        /**
         * Set the unique identification number for the actor instance.
         * @param[in] id The unique identification number.
         */
        void SetID(uint32 id);

        /**
         * Get the motion system of this actor instance.
         * The motion system handles all the motion management and blending. If you want to play a motion or figure out
         * what motions are currently active, you have to use the motion system class.
         * @result A pointer to the motion system.
         */
        MotionSystem* GetMotionSystem() const;

        /**
         * Set the current motion system to use.
         * On default a MotionLayerSystem is set.
         * @param newSystem The new motion system to use.
         * @param delCurrentFromMem If set to true, the currently set motion system will be deleted from memory, before setting the new motion system.
         */
        void SetMotionSystem(MotionSystem* newSystem, bool delCurrentFromMem = true);

        /**
         * Get the anim graph instance.
         * This can return nullptr, in which case the motion system as returned by GetMotionSystem() will be used.
         * @result The anim graph instance.
         */
        MCORE_INLINE AnimGraphInstance* GetAnimGraphInstance() const          { return m_animGraphInstance; }

        /**
         * Set the anim graph instance.
         * This can be nullptr, in which case the motion system as returned by GetMotionSystem() will be used.
         * @param instance The anim graph instance.
         */
        void SetAnimGraphInstance(AnimGraphInstance* instance);

        /**
         * Get the transformation data class for this actor instance.
         * This transformation data class gives you access to all the transforms of the nodes in the actor.
         * So if you wish to get or set any transformations, you can do it with the object returned by this method.
         * @result A pointer to the transformation data object.
         */
        MCORE_INLINE TransformData* GetTransformData() const                    { return m_transformData; }

        /**
         * Enable or disable this actor instance.
         * Disabled actor instances are not processed at all. It will be like they do not exist.
         * On default the actor instance is enabled, after creation.
         * You can disable an actor instance that acts as skin attachment, but is not currently attached. This way the clothing items your character doesn't wear won't take processing power.
         * It is always faster not to have the actor instance created at all in such case though.
         * @param enabled Specifies whether this actor instance is enabled or not.
         */
        void SetIsEnabled(bool enabled);

        /**
         * Check whether this actor instance is enabled or not.
         * Disabled actor instances are not updated and processed.
         * @result Returns true when enabled, or false when disabled.
         */
        MCORE_INLINE bool GetIsEnabled() const                                  { return (m_boolFlags & BOOL_ENABLED) != 0; }

        /**
         * Check the visibility flag.
         * This flag has been set by the user and identifies if this actor instance is visible or not.
         * This is used internally by the schedulers, so that heavy calculations can be skipped on invisible characters.
         * @result Returns true when the actor instance is marked as visible, otherwise false is returned.
         */
        MCORE_INLINE bool GetIsVisible() const                                  { return (m_boolFlags & BOOL_ISVISIBLE) != 0; }

        /**
         * Change the visibility state.
         * @param isVisible Set to true when the actor instance is visible, otherwise set it to false.
         */
        void SetIsVisible(bool isVisible);

        /**
         * Specifies if this actor instance is visible or not.
         * This recursively propagates its visibility status to child attachments.
         * So if your horse is visibe then the rider that is attached on top of the horse will also become marked as visible.
         * @param Specify if all nodes touched should be marked as visible or not.
         */
        void RecursiveSetIsVisible(bool isVisible);


        /**
         * Recursively set the actor instance visibility flag, upwards in hierarchy, moving from an attachment up to the root actor instance.
         * @param Specify if all nodes touched should be marked as visible or not.
         */
        void RecursiveSetIsVisibleTowardsRoot(bool isVisible);

        //------------------------------------------------

        /**
         * Use the skeletal LOD flags from the nodes of the corresponding actor and pass them over to this actor instance.
         * The actor keeps the information about which nodes are enabled at what skeletal LOD and this is the transfer function to actually apply it to the actor instance
         * as each actor instance can be in a different skeletal LOD level.
         */
        void UpdateSkeletalLODFlags();

        /**
         * Calculate the number of disabled nodes for a given skeletal lod level.
         * @param[in] skeletalLODLevel The skeletal LOD level to calculate the number of disabled nodes for.
         * @return  The number of disabled nodes for the given skeletal LOD level.
         */
        size_t CalcNumDisabledNodes(size_t skeletalLODLevel) const;

        /**
         * Calculate the number of used skeletal LOD levels. Each actor instance alsways has 32 skeletal LOD levels while in most cases
         * not all of them are actually used. This function determines the number of skeletal LOD levels that actually disable some more nodes
         * relative to the previous LOD level.
         * @return The number of actually used skeletal LOD levels.
         */
        size_t CalcNumSkeletalLODLevels() const;

        /**
         * Get the current used geometry and skeletal detail level.
         * In total there are 32 possible skeletal LOD levels, where 0 is the highest detail, and 31 the lowest detail.
         * Each detail level can disable specific nodes in the actor. These disabled nodes will not be processed at all by EMotion FX.
         * It is important to know that disabled nodes never should be used inside skinning information or on other places where their transformations
         * are needed.
         * @result The current LOD level.
         */
        size_t GetLODLevel() const;

        /**
         * Set the current geometry and skeletal detail level, where 0 is the highest detail.
         * @param level The LOD level. Values higher than [GetNumGeometryLODLevels()-1] will be clamped to the maximum LOD.
         */
        void SetLODLevel(size_t level);

        //--------------------------------

        /**
         * Get the entity to which the given actor instance belongs to.
         * @result Entity having an actor component added to it to which this actor instance belongs to. nullptr in case the actor instance is used without the entity component system.
         */
        AZ::Entity* GetEntity() const;

        /**
         * Get the entity id to which the given actor instance belongs to.
         * @result Entity id of the actor instance. EntityId::InvalidEntityId in case the actor instance is used without the entity component system.
         */
        AZ::EntityId GetEntityId() const;

        /**
         * Set a pointer to some custom data you want to store and link with this actor instance object.
         * Custom data can for example link a game or engine object with this EMotion FX ActorInstance object.
         * An example is when EMotion FX triggers a motion event. You know the actor that triggered the event, but
         * you don't know directly what game or engine object is linked to this actor.
         * By using the custom data methods GetCustomData and SetCustomData you can set a pointer to your game or engine
         * object in each actor instance.
         * The pointer that you specify will not be deleted when the actor instance is being destructed.
         * @param customData A void pointer to the custom data to link with this actor instance.
         */
        void SetCustomData(void* customData);

        /**
         * Get a pointer to the custom data you stored and linked with this actor instance object.
         * Custom data can for example link a game or engine object with this EMotion FX ActorInstance object.
         * An example is when EMotion FX triggers a motion event. You know the actor that triggered the event, but
         * you don't know directly what game or engine object is linked to this actor.
         * By using the custom data methods GetCustomData and SetCustomData you can set a pointer to your game or engine
         * object in each actor instance.
         * The pointer that you specify will not be deleted when the actor instance is being destructed.
         * @result A void pointer to the custom data you have specified.
         */
        void* GetCustomData() const;

        //-------------------------------------------------------------------------------------------

        // misc / partial update methods
        /**
         * Apply the morph targets transforms additively to the current local transforms as they are stored inside
         * the TransformData object that you retrieve with GetTransformData().
         * This will not apply any mesh morphs yet, as that is performed by the UpdateMeshDeformers() method.
         */
        void ApplyMorphSetup();

        void UpdateWorldTransform();

        /**
         * Update the skinning matrices.
         * This will update the data inside the TransformData class.
         */
        void UpdateSkinningMatrices();

        /**
         * Update all the attachments.
         * This calls the update method for each attachment.
         */
        void UpdateAttachments();

        // main methods
        /**
         * Update the transformations of this actor instance. This can be the actor instance transform and can also include the joint transforms.
         * This automatically updates all motion timers and anim graph nodes as well.
         * @param timePassedInSeconds The time passed in seconds, since the last frame or update.
         * @param updateJointTransforms When set to true the joint transformations will be calculated by calculating the animation graph output for example.
         * @param sampleMotions When set to true motions will be sampled, or whole anim graphs if using those. When updateMatrices is set to false, motions will never be sampled, even if set to true.
         */
        void UpdateTransformations(float timePassedInSeconds, bool updateJointTransforms = true, bool sampleMotions = true);

        /**
         * Update/Process the mesh deformers.
         * This will apply skinning and morphing deformations to the meshes used by the actor instance.
         * All deformations happen on the CPU. So if you use pure GPU processing, you should not be calling this method.
         * Also invisible actor instances should not update their mesh deformers, as this can be very CPU intensive.
         * @param timePassedInSeconds The time passed in seconds, since the last frame or update.
         * @param processDisabledDeformers When set to true, even mesh deformers that are disabled will even be processed.
         */
        void UpdateMeshDeformers(float timePassedInSeconds, bool processDisabledDeformers = false);

        /**
        * Update/Process the morph mesh deformers.
        * This will apply morphing deformations only to the meshes used by the actor instance.
        * All morph deformations happen on the CPU. So if you use pure GPU processing, you should not be calling this method.
        * @param timePassedInSeconds The time passed in seconds, since the last frame or update.
        * @param processDisabledDeformers When set to true, even mesh deformers that are disabled will even be processed.
        */
        void UpdateMorphMeshDeformers(float timePassedInSeconds, bool processDisabledDeformers = false);

        void PostPhysicsUpdate(float timePassedInSeconds);

        //-------------------------------------------------------------------------------------------

        // bounding volume
        /**
         * Setup the automatic update settings of the bounding volume.
         * This allows you to specify at what time interval the bounding volume of the actor instance should be updated and
         * if this update should base its calculations on the nodes, mesh vertices or collision mesh vertices.
         * @param updateFrequencyInSeconds The bounds will be updated every "updateFrequencyInSeconds" seconds. A value of 0 would mean every frame and a value of 0.1 would mean 10 times per second.
         * @param boundsType The type of bounds calculation. This can be either based on the node's world space positions, the mesh world space positions or the collision mesh world space positions.
         * @param itemFrequency A value of 1 would mean every node or vertex will be taken into account in the bounds calculation.
         *                      A value of 2 would mean every second node or vertex would be taken into account. A value of 5 means every 5th node or vertex, etc.
         *                      Higher values will result in faster bounds updates, but also in possibly less accurate bounds. This value must 1 or higher. Zero is not allowed.
         */
        void SetupAutoBoundsUpdate(float updateFrequencyInSeconds, EBoundsType boundsType = BOUNDS_NODE_BASED, uint32 itemFrequency = 1);

        /**
         * Check if the automatic bounds update feature is enabled.
         * It will use the settings as provided to the SetupAutoBoundsUpdate method.
         * @result Returns true when the automatic bounds update feature is enabled, or false when it is disabled.
         */
        bool GetBoundsUpdateEnabled() const;

        /**
         * Get the automatic bounds update time frequency.
         * This specifies the time interval between bounds updates. A value of 0.1 would mean that the bounding volumes
         * will be updated 10 times per second. A value of 0.25 means 4 times per second, etc. A value of 0 can be used to
         * force the updates to happen every frame.
         * @result Returns the automatic bounds update frequency time interval, in seconds.
         */
        float GetBoundsUpdateFrequency() const;

        /**
         * Get the time passed since the last automatic bounds update.
         * @result The time passed, in seconds, since the last automatic bounds update.
         */
        float GetBoundsUpdatePassedTime() const;

        /**
         * Get the bounding volume auto-update type.
         * This can be either based on the node's world space positions, the mesh vertex world space positions, or the
         * collision mesh vertex world space postitions.
         * @result The bounding volume update type.
         */
        EBoundsType GetBoundsUpdateType() const;

        /**
         * Get the normalized percentage that the calculated bounding box is expanded with.
         * This can be used to add a tolerance area to the calculated bounding box to avoid clipping the character too early.
         * A static bounding box together with the expansion is the recommended way for maximum performance.
         * @result A value of 0.0 means that the calculated bounding box won't be expanded at all, while .25 means it will become 125% the original size.
         */
        float GetExpandBoundsBy() const { return m_boundsExpandBy; }

        /**
        * Expand a bounding box by a given percentage
        * @param aabb The bounding box to expand
        * @param expandBoundsBy Percentage to expand the bounds by. A value of 0.0 means that the calculated bounding box won't be expanded at all, while .25 means it will become 125% the original size.
        */
        static void ExpandBounds(AZ::Aabb& aabb, float expandByPercentage);

        /**
         * Get the bounding volume auto-update item frequency.
         * A value of 1 would mean every node or vertex will be taken into account in the bounds calculation.
         * A value of 2 would mean every second node or vertex would be taken into account. A value of 5 means every 5th node or vertex, etc.
         * Higher values will result in faster bounds updates, but also in possibly less accurate bounds.
         * The value returned will always be equal or greater than one.
         * @result The item frequency, which will be equal or greater than one.
         */
        uint32 GetBoundsUpdateItemFrequency() const;

        /**
         * Set the auto-bounds update time frequency, in seconds.
         * This specifies the time interval between bounds updates. A value of 0.1 would mean that the bounding volumes
         * will be updated 10 times per second. A value of 0.25 means 4 times per second, etc. A value of 0 can be used to
         * force the updates to happen every frame.
         * @param seconds The amount of seconds between every bounding volume update.
         */
        void SetBoundsUpdateFrequency(float seconds);

        /**
         * Set the time passed since the last automatic bounds update.
         * @param seconds The amount of seconds, since the last automatic update.
         */
        void SetBoundsUpdatePassedTime(float seconds);

        /**
         * Set the bounding volume auto-update type.
         * This can be either based on the node's world space positions, the mesh vertex world space positions, or the
         * collision mesh vertex world space positions.
         * @param bType The bounding volume update type.
         */
        void SetBoundsUpdateType(EBoundsType bType);

        /**
         * Set the normalized percentage that the calculated bounding box should be expanded with.
         * This can be used to add a tolerance area to the calculated bounding box to avoid clipping the character too early.
         * A static bounding box together with the expansion is the recommended way for maximum performance.
         * @param[in] expandBy A value of 1.0 means that the calculated bounding box won't be expanded at all, while 2.0 means it will be twice the size.
         */
        void SetExpandBoundsBy(float expandBy) { m_boundsExpandBy = expandBy; }

        /**
         * Set the bounding volume auto-update item frequency.
         * A value of 1 would mean every node or vertex will be taken into account in the bounds calculation.
         * A value of 2 would mean every second node or vertex would be taken into account. A value of 5 means every 5th node or vertex, etc.
         * Higher values will result in faster bounds updates, but also in possibly less accurate bounds.
         * The frequency value must always be greater than or equal to one.
         * @param freq The item frequency, which has to be 1 or above. Higher values result in faster performance, but lower accuracy.
         */
        void SetBoundsUpdateItemFrequency(uint32 freq);

        /**
         * Specify whether you want the auto-bounds update to be enabled or disabled.
         * On default, after creation of the actor instance it is enabled, using node based, with an update frequency of 1 and
         * an update frequency of 0.1 (ten times per second).
         * @param enable Set to true when you want to enable the bounds automatic update feature, or false when you'd like to disable it.
         */
        void SetBoundsUpdateEnabled(bool enable);

        /**
         * Update the bounding volumes of the actor.
         * @param geomLODLevel The geometry level of detail number to generate the LOD for. Must be in range of 0..GetNumGeometryLODLevels()-1.
         *                     The same LOD level will be chosen for the attachments when they will be included.
         *                     If the specified LOD level is lower than the number of attachment LODS, the lowest attachment LOD will be chosen.
         * @param boundsType The bounding volume generation method, each having different accuracies and generation speeds.
         * @param itemFrequency Depending on the type of bounds you specify to the boundsType parameter, this specifies how many "vertices or nodes" the
         *                      updating functions should skip. If you specified a value of 4, and use BOUND_MESH_BASED or collision mesh based, every
         *                      4th vertex will be included in the bounds calculation, so only processing 25% of the total number of vertices. The same goes for
         *                      node based bounds, but then it will process every 4th node. Of course higher values produce less accurate results, but are faster to process.
         */
        void UpdateBounds(size_t geomLODLevel, EBoundsType boundsType = BOUNDS_NODE_BASED, uint32 itemFrequency = 1);

        /**
         * Update the base static axis aligned bounding box shape.
         * This is a quite heavy function in comparision to the CalcStaticBasedAABB. Basically the dimensions of the static based aabb are calculated here.
         * First it will try to generate the aabb from the meshes. If there are no meshes it will use a node based aabb as basis.
         * After that it will find the maximum of the depth, width and height, and makes all dimensions the same as this max value.
         * This function is generally only executed once, when creating the actor instance.
         * The CalcStaticBasedAABB function then simply translates this box along with the actor instance's position.
         */
        void UpdateStaticBasedAabbDimensions();

        void SetStaticBasedAabb(const AZ::Aabb& aabb);
        void GetStaticBasedAabb(AZ::Aabb* outAabb);
        const AZ::Aabb& GetStaticBasedAabb() const;

        /**
         * Calculate an axis aligned bounding box that can be used as static AABB. It is static in the way that the volume does not change. It can however be translated as it will move
         * with the character's position. The box will contain the mesh based bounds, and finds the maximum of the width, depth and height, and makes all depth width and height equal to this value.
         * So the box will in most cases be too wide, but this is done on purpose. This can be used when the character is outside of the frustum, but we still update the position.
         * We can then use this box to test for visibility again.
         * If there are no meshes present, a widened node based box will be used instead as basis.
         * @param outResult The resulting bounding box, moved along with the actor instance's position.
         */
        void CalcStaticBasedAabb(AZ::Aabb* outResult);

        /**
         * Calculate the axis aligned bounding box based on the world space positions of the nodes.
         * @param outResult The AABB where this method should store the resulting box in.
         * @param nodeFrequency This will include every "nodeFrequency"-th node. So a value of 1 will include all nodes. A value of 2 would
         *                      process every second node, meaning that half of the nodes will be skipped. A value of 4 would process every 4th node, etc.
         */
        void CalcNodeBasedAabb(AZ::Aabb* outResult, uint32 nodeFrequency = 1);

        /**
         * Calculate the axis aligned bounding box based on the world space vertex coordinates of the meshes.
         * If the actor has no meshes, the created box will be invalid.
         * @param geomLODLevel The geometry LOD level to calculate the box for.
         * @param outResult The AABB where this method should store the resulting box in.
         * @param vertexFrequency This includes every "vertexFrequency"-th vertex. So for example a value of 2 would skip every second vertex and
         *                        so will process half of the vertices. A value of 4 would process only each 4th vertex, etc.
         */
        void CalcMeshBasedAabb(size_t geomLODLevel, AZ::Aabb* outResult, uint32 vertexFrequency = 1);

        /**
         * Get the axis aligned bounding box.
         * This box will be updated once the UpdateBounds method is called.
         * That method is also called automatically when the bounds auto-update feature is enabled.
         * @result The axis aligned bounding box.
         */
        const AZ::Aabb& GetAabb() const;

        /**
         * Set the axis aligned bounding box.
         * Please beware that this box will get automatically overwritten when automatic bounds update is enabled.
         * @param aabb The axis aligned bounding box to store.
         */
        void SetAabb(const AZ::Aabb& aabb);

        //-------------------------------------------------------------------------------------------

        /**
         * Set the local space position of this actor instance.
         * This is relative to its parent (if it is attached ot something). Otherwise it is in world space.
         * @param position The position/translation to use.
         */
        MCORE_INLINE void SetLocalSpacePosition(const AZ::Vector3& position)          { m_localTransform.m_position = position; }

        /**
         * Set the local rotation of this actor instance.
         * This is relative to its parent (if it is attached ot something). Otherwise it is in world space.
         * @param rotation The rotation to use.
         */
        MCORE_INLINE void SetLocalSpaceRotation(const AZ::Quaternion& rotation)    { m_localTransform.m_rotation = rotation; }

        EMFX_SCALECODE
        (
            /**
             * Set the local scale of this actor instance.
             * This is relative to its parent (if it is attached ot something). Otherwise it is in world space.
             * @param scale The scale to use.
             */
            MCORE_INLINE void SetLocalSpaceScale(const AZ::Vector3& scale)           { m_localTransform.m_scale = scale; }

            /**
             * Get the local space scale.
             * This is relative to its parent (if it is attached ot something). Otherwise it is in world space.
             * @result The local space scale factor for each axis.
             */
            MCORE_INLINE const AZ::Vector3& GetLocalSpaceScale() const              { return m_localTransform.m_scale; }
        )

        /**
         * Get the local space position/translation of this actor instance.
         * This is relative to its parent (if it is attached ot something). Otherwise it is in world space.
         * @result The local space position.
         */
        MCORE_INLINE const AZ::Vector3& GetLocalSpacePosition() const               { return m_localTransform.m_position; }

        /**
         * Get the local space rotation of this actor instance.
         * This is relative to its parent (if it is attached ot something). Otherwise it is in world space.
         * @result The local space rotation.
         */
        MCORE_INLINE const AZ::Quaternion& GetLocalSpaceRotation() const         { return m_localTransform.m_rotation; }

        MCORE_INLINE void SetLocalSpaceTransform(const Transform& transform)        { m_localTransform = transform; }

        MCORE_INLINE const Transform& GetLocalSpaceTransform() const                { return m_localTransform; }
        MCORE_INLINE const Transform& GetWorldSpaceTransform() const                { return m_worldTransform; }
        MCORE_INLINE const Transform& GetWorldSpaceTransformInversed() const        { return m_worldTransformInv; }

        //-------------------------------------------------------------------------------------------

        // attachments
        /**
         * Check if we can safely attach an attachment that uses the specified actor instance.
         * This will check for infinite recursion/circular chains.
         * @param attachmentInstance The actor instance we are trying to attach to this actor instance.
         * @result Returns true when we can safely attach the specified actor instance, otherwise false is returned.
         */
        bool CheckIfCanHandleAttachment(const ActorInstance* attachmentInstance) const;

        /**
         * Check if this actor instance has a specific attachment that uses a specified actor instance.
         * This function is recursive, so it also checks the attachments of the attachments, etc.
         * @param attachmentInstance The actor instance you want to check with, which represents the attachment.
         * @result Returns true when the specified actor instance is being used as attachment down the hierarchy, otherwise false is returned.
         */
        bool RecursiveHasAttachment(const ActorInstance* attachmentInstance) const;

        /**
         * Add an attachment to this actor.
         * Please note that each attachment can only belong to one actor instance.
         * @param attachment The attachment to add.
         */
        void AddAttachment(Attachment* attachment);

        /**
         * Remove a given attachment.
         * @param nr The attachment number, which must be in range of [0..GetNumAttachments()-1].
         * @param delFromMem Set this to true when you want the attachment that gets removed also to be deleted from memory.
         *                   When you set this to false, it will not be deleted from memory, but only removed from the array of attachments
         *                   that is stored locally inside this actor instance.
         */
        void RemoveAttachment(size_t nr, bool delFromMem = true);

        /**
         * Remove all attachments from this actor instance.
         * @param delFromMem Set this to true when you want the attachments also to be deleted from memory.
         *                   When you set this to false, it will not be deleted from memory, but only removed from the array of attachments
         *                   that is stored locally inside this actor instance.
         */
        void RemoveAllAttachments(bool delFromMem = true);

        /**
         * Remove an attachment that uses a specified actor instance.
         * @param actorInstance The actor instance that the attachment is using.
         * @param delFromMem Set this to true when you want the attachment also to be deleted from memory.
         *                   When you set this to false, it will not be deleted from memory, but only removed from the array of attachments
         *                   that is stored locally inside this actor instance.
         */
        bool RemoveAttachment(ActorInstance* actorInstance, bool delFromMem = true);

        /**
         * Find the attachment number that uses a given actor instance.
         * @param actorInstance The actor instance that the attachment you search for is using.
         * @result Returns the attachment number, in range of [0..GetNumAttachments()-1], or MCORE_INVALIDINDEX32 when no attachment
         *         using the specified actor instance can be found.
         */
        size_t FindAttachmentNr(ActorInstance* actorInstance);

        /**
         * Get the number of attachments that have been added to this actor instance.
         * @result The number of attachments added to this actor instance.
         */
        size_t GetNumAttachments() const;

        /**
         * Get a specific attachment.
         * @param nr The attachment number, which must be in range of [0..GetNumAttachments()-1].
         * @result A pointer to the attachment.
         */
        Attachment* GetAttachment(size_t nr) const;

        /**
         * Check whether this actor instance also is an attachment or not.
         * So if this actor instance is like a cowboy, and the cowboy is attached to a horse, then this will return true.
         * If the cowboy (so this actor instance) would not be attached to anything, it will return false.
         * @result Returns true when this actor instance is also an attachment to some other actor instance. Otherwise false is returned.
         */
        bool GetIsAttachment() const;

        /**
         * Get the actor instance where this actor instance is attached to.
         * This will return nullptr in case this actor instance is not an attachment.
         * @result A pointer to the actor instance where this actor instance is attached to.
         */
        ActorInstance* GetAttachedTo() const;

        /**
         * Find the root actor instance.
         * If this actor instance object represents a gun, which is attached to a cowboy, which is attached to a horse, then
         * the attachment root that is returned by this method is the horse.
         * @result The attachment root, or a pointer to itself when this actor instance isn't attached to anything.
         */
        ActorInstance* FindAttachmentRoot() const;

        /**
         * Get the attachment where this actor instance is part of.
         * So if this actor instance is a gun, and the gun is attached to some cowboy actor instance, then the Attachment object that
         * is returned here, is the attachment object for the gun that was added to the cowboy actor instance.
         * @result The attachment where this actor instance takes part of, or nullptr when this actor instance isn't an attachment.
         */
        Attachment* GetSelfAttachment() const;

        /**
         * Check if the actor instance is a skin attachment.
         * @result Returns true when the actor instance itself is a skin attachment, otherwise false is returned.
         */
        bool GetIsSkinAttachment() const;

        //-------------------------------------------------------------------------------------------

        /**
         * Update all dependencies of this actor instance.
         */
        void UpdateDependencies();

        /**
         * Recursively add dependencies for the given actor.
         * This will add all dependencies stored in the specified actor, to this actor instance.
         * Also it will recurse into the dependencies of the dependencies of the given actor.
         * @param actor The actor we should recursively copy the dependencies from.
         */
        void RecursiveAddDependencies(Actor* actor);

        /**
         * Get the number of dependencies that this actor instance has on other actors.
         * @result The number of dependencies.
         */
        size_t GetNumDependencies() const;

        /**
         * Get a given dependency.
         * @param nr The dependency number to get, which must be in range of [0..GetNumDependencies()].
         * @result A pointer to the dependency.
         */
        Actor::Dependency* GetDependency(size_t nr);

        /**
         * Get the morph setup instance.
         * This doesn't contain the actual morph targets, but the unique settings per morph target.
         * These unique settings are the weight value per morph target, and a boolean that specifies if the morph target
         * weight should be controlled automatically (extracted from motions) or manually by user specified values.
         * If you want to access the actual morph targets, you have to use the Actor::GetMorphSetup() method.
         * When the morph setup instance object doesn't contain any morph targets, no morphing is used.
         * @result A pointer to the morph setup instance object for this actor instance.
         */
        MorphSetupInstance* GetMorphSetupInstance() const;

        /**
         * Check for an intersection between the collision mesh of this actor and a given ray.
         * Returns a pointer to the node it detected a collision in case there is a collision with any of the collision meshes of all nodes of this actor.
         * If there is no collision mesh attached to the nodes, no intersection test will be done, and nullptr will be returned.
         * @param lodLevel The level of detail to check the collision with.
         * @param ray The ray to check.
         * @return A pointer to the node we detected the first intersection with (doesn't have to be the closest), or nullptr when no intersection found.
         */
        Node* IntersectsCollisionMesh(size_t lodLevel, const MCore::Ray& ray) const;

        /**
         * Check for an intersection between the collision mesh of this actor and a given ray, and calculate the closest intersection point.
         * If there is no collision mesh attached to the nodes, no intersection test will be done, and nullptr will be returned.
         * Returns a pointer to the node it detected a collision in case there is a collision with the collision meshes of the actor, 'outIntersect'
         * will contain the closest intersection point in case there is a collision. Use the other Intersects method when you do not need the intersection
         * point (since that one is faster).
         * @param lodLevel The level of detail to check the collision with.
         * @param ray The ray to test with.
         * @param outIntersect A pointer to the vector to store the intersection point in, in case of a collision (nullptr allowed).
         * @param outNormal A pointer to the vector to store the normal at the intersection point, in case of a collision (nullptr allowed).
         * @param outUV A pointer to the vector to store the uv coordinate at the intersection point (nullptr allowed).
         * @param outBaryU A pointer to a float in which the method will store the barycentric U coordinate, to be used to interpolate values on the triangle (nullptr allowed).
         * @param outBaryV A pointer to a float in which the method will store the barycentric V coordinate, to be used to interpolate values on the triangle (nullptr allowed).
         * @param outIndices A pointer to an array of 3 integers, which will contain the 3 vertex indices of the closest intersecting triangle. Even on polygon meshes with polygons of more than 3 vertices three indices are returned. In that case the indices represent a sub-triangle inside the polygon.
         *                   A value of nullptr is allowed, which will skip storing the resulting triangle indices.
         * @return A pointer to the node we detected the closest intersection with, or nullptr when no intersection found.
         */
        Node* IntersectsCollisionMesh(size_t lodLevel, const MCore::Ray& ray, AZ::Vector3* outIntersect, AZ::Vector3* outNormal = nullptr, AZ::Vector2* outUV = nullptr, float* outBaryU = nullptr, float* outBaryV = nullptr, uint32* outIndices = nullptr) const;

        /**
         * Check for an intersection between the real mesh (if present) of this actor and a given ray.
         * Returns a pointer to the node it detected a collision in case there is a collision with any of the real meshes of all nodes of this actor.
         * If there is no mesh attached to this node, no intersection test will be performed and nullptr will be returned.
         * @param lodLevel The level of detail to check the collision with.
         * @param ray The ray to test with.
         * @return Returns a pointer to itself when an intersection occurred, or nullptr when no intersection found.
         */
        Node* IntersectsMesh(size_t lodLevel, const MCore::Ray& ray) const;

        /**
         * Checks for an intersection between the real mesh (if present) of this actor and a given ray.
         * Returns a pointer to the node it detected a collision in case there is a collision with any of the real meshes of all nodes of this actor,
         * 'outIntersect' will contain the closest intersection point in case there is a collision.
         * Both the intersection point and normal which are returned are in world space.
         * Use the other Intersects method when you do not need the intersection point (since that one is faster).
         * Both the intersection point and normal which are returned are in world space.
         * @param lodLevel The level of detail to check the collision with.
         * @param ray The ray to test with.
         * @param outIntersect A pointer to the vector to store the intersection point in, in case of a collision (nullptr allowed).
         * @param outNormal A pointer to the vector to store the normal at the intersection point, in case of a collision (nullptr allowed).
         * @param outUV A pointer to the vector to store the uv coordinate at the intersection point (nullptr allowed).
         * @param outBaryU A pointer to a float in which the method will store the barycentric U coordinate, to be used to interpolate values on the triangle (nullptr allowed).
         * @param outBaryV A pointer to a float in which the method will store the barycentric V coordinate, to be used to interpolate values on the triangle (nullptr allowed).
         * @param outIndices A pointer to an array of 3 integers, which will contain the 3 vertex indices of the closest intersecting triangle. Even on polygon meshes with polygons of more than 3 vertices three indices are returned. In that case the indices represent a sub-triangle inside the polygon.
         *                   A value of nullptr is allowed, which will skip storing the resulting triangle indices.
         * @return A pointer to the node we detected the closest intersection with, or nullptr when no intersection found.
         */
        Node* IntersectsMesh(size_t lodLevel, const MCore::Ray& ray, AZ::Vector3* outIntersect, AZ::Vector3* outNormal = nullptr, AZ::Vector2* outUV = nullptr, float* outBaryU = nullptr, float* outBaryV = nullptr, uint32* outStartIndex = nullptr) const;

        void SetRagdoll(Physics::Ragdoll* ragdoll);
        RagdollInstance* GetRagdollInstance() const;

        void SetParentWorldSpaceTransform(const Transform& transform);
        const Transform& GetParentWorldSpaceTransform() const;

        /**
         * Set whether calls to ActorUpdateCallBack::OnRender() for this actor instance should be made or not.
         * @param enabled Set to true when the ActorUpdateCallBack::OnRender() should be called for this actor instance, otherwise set to false.
         */
        void SetRender(bool enabled);

        /**
         * Check if calls to ActorUpdateCallBack::OnRender() for this actor instance are being made or not.
         * @result Returns true when the ActorUpdateCallBack::OnRender() is being called for this actor instance. False is returned in case this callback won't be executed for this actor instance.
         */
        bool GetRender() const;

        void SetIsUsedForVisualization(bool enabled);
        bool GetIsUsedForVisualization() const;

        /**
         * Marks the actor instance as used by the engine runtime, as opposed to the tool suite.
         */
        void SetIsOwnedByRuntime(bool isOwnedByRuntime);
        bool GetIsOwnedByRuntime() const;

        /**
         * Enable a specific node.
         * This will activate motion sampling, transformation and blending calculations for the given node.
         * @param nodeIndex The node number to enable. This must be in range of [0..Actor::GetNumNodes()-1].
         */
        void EnableNode(uint16 nodeIndex);

        /**
         * Disable a specific node.
         * This will disable motion sampling, transformation and blending calculations for the given node.
         * @param nodeIndex The node number to disable. This must be in range of [0..Actor::GetNumNodes()-1].
         */
        void DisableNode(uint16 nodeIndex);

        /**
         * Get direct access to the array of enabled nodes.
         * @result A read only reference to the array of enabled nodes. The values inside of this array are the node numbers of the enabled nodes.
         */
        MCORE_INLINE const AZStd::vector<uint16>& GetEnabledNodes() const        { return m_enabledNodes; }

        /**
         * Get the number of enabled nodes inside this actor instance.
         * @result The number of nodes that have been enabled and are being updated.
         */
        MCORE_INLINE size_t GetNumEnabledNodes() const                          { return m_enabledNodes.size(); }

        /**
         * Get the node number of a given enabled node.
         * @param index An index in the array of enabled nodes. This must be in range of [0..GetNumEnabledNodes()-1].
         * @result The node number, which relates to Actor::GetNode( returnValue ).
         */
        MCORE_INLINE uint16 GetEnabledNode(size_t index) const                  { return m_enabledNodes[index]; }

        /**
         * Enable all nodes inside the actor instance.
         * This means that all nodes will be processed and will have their motions sampled (unless disabled by LOD), local and world space matrices calculated, etc.
         * After actor instance creation time it is possible that some nodes are disabled on default. This is controlled by node groups (represented by the NodeGroup class).
         * Each Actor object has a set of node groups. Each group contains a set of nodes which are either enabled or disabled on default.
         * You can use the Actor::GetNodeGroup(...) related methods to get access to the groups and enable or disable the predefined groups of nodes manually.
         */
        void EnableAllNodes();

        /**
         * Disable all nodes inside the actor instance.
         * This means that no nodes will be updated at all (no motion sampling, no transformation calculations and no blending etc).
         * After actor instance creation time it is possible that some nodes are disabled on default. This is controlled by node groups (represented by the NodeGroup class).
         * Each Actor object has a set of node groups. Each group contains a set of nodes which are either enabled or disabled on default.
         * You can use the Actor::GetNodeGroup(...) related methods to get access to the groups and enable or disable the predefined groups of nodes manually.
         */
        void DisableAllNodes();

        uint32 GetThreadIndex() const;
        void SetThreadIndex(uint32 index);

        void DrawSkeleton(const Pose& pose, const AZ::Color& color);
        static void ApplyMotionExtractionDelta(Transform& inOutTransform, const Transform& trajectoryDelta);
        void ApplyMotionExtractionDelta(const Transform& trajectoryDelta);
        void ApplyMotionExtractionDelta();
        void MotionExtractionCompensate(EMotionExtractionFlags motionExtractionFlags = (EMotionExtractionFlags)0);
        void MotionExtractionCompensate(Transform& inOutMotionExtractionNodeTransform,
            EMotionExtractionFlags motionExtractionFlags = (EMotionExtractionFlags)0) const;

        /**
         * Remove the trajectory transform from the input transformation.
         * @param[out] inOutMotionExtractionNodeTransform Local space transformation of the motion extraction joint.
         * @param[in] localSpaceBindPoseTransform Bind pose transform of the motion extraction joint in local space.
         * @param[in] motionExtractionFlags Motion extraction capture options.
         */
        static void MotionExtractionCompensate(Transform& inOutMotionExtractionNodeTransform,
            const Transform& localSpaceBindPoseTransform,
            EMotionExtractionFlags motionExtractionFlags = (EMotionExtractionFlags)0);

        void SetMotionExtractionEnabled(bool enabled);
        bool GetMotionExtractionEnabled() const;

        void SetTrajectoryDeltaTransform(const Transform& transform);
        const Transform& GetTrajectoryDeltaTransform() const;

        AnimGraphPose* RequestPose(uint32 threadIndex);
        void FreePose(uint32 threadIndex, AnimGraphPose* pose);

        void SetMotionSamplingTimer(float timeInSeconds);
        void SetMotionSamplingRate(float updateRateInSeconds);
        float GetMotionSamplingTimer() const;
        float GetMotionSamplingRate() const;

        MCORE_INLINE size_t GetNumNodes() const         { return m_actor->GetSkeleton()->GetNumNodes(); }

        void UpdateVisualizeScale();                    // not automatically called on creation for performance reasons (this method relatively is slow as it updates all meshes)
        float GetVisualizeScale() const;
        void SetVisualizeScale(float factor);
        MCORE_INLINE void SetLightingChannelMask(uint32_t lightingChannelMask){ m_lightingChannelMask = lightingChannelMask; }
        MCORE_INLINE uint32_t GetLightingChannelMask() const { return m_lightingChannelMask; }

    private:
        TransformData*          m_transformData;         /**< The transformation data for this instance. */
        AZ::Aabb                m_aabb;                  /**< The axis aligned bounding box. */
        AZ::Aabb                m_staticAabb;           /**< A static pre-calculated bounding box, which we can move along with the position of the actor instance, and use for visibility checks. */

        Transform               m_localTransform = Transform::CreateIdentity();
        Transform               m_worldTransform = Transform::CreateIdentity();
        Transform               m_worldTransformInv = Transform::CreateIdentity();
        Transform               m_parentWorldTransform = Transform::CreateIdentity();
        Transform               m_trajectoryDelta = Transform::CreateIdentityWithZeroScale();

        AZStd::vector<Attachment*>               m_attachments;       /**< The attachments linked to this actor instance. */
        AZStd::vector<Actor::Dependency>         m_dependencies;      /**< The actor dependencies, which specify which Actor objects this instance is dependent on. */
        MorphSetupInstance*                     m_morphSetup;        /**< The  morph setup instance. */
        AZStd::vector<uint16>                    m_enabledNodes;      /**< The list of nodes that are enabled. */

        Actor*                  m_actor;                 /**< A pointer to the parent actor where this is an instance from. */
        ActorInstance*          m_attachedTo;            /**< Specifies the actor where this actor is attached to, or nullptr when it is no attachment. */
        Attachment*             m_selfAttachment;        /**< The attachment it is itself inside the m_attachedTo actor instance, or nullptr when this isn't an attachment. */
        MotionSystem*           m_motionSystem;          /**< The motion system, that handles all motion playback and blending etc. */
        AnimGraphInstance*      m_animGraphInstance;     /**< A pointer to the anim graph instance, which can be nullptr when there is no anim graph instance. */
        AZStd::unique_ptr<RagdollInstance> m_ragdollInstance;
        MCore::Mutex            m_lock;                  /**< The multi-thread lock. */
        void*                   m_customData;            /**< A pointer to custom data for this actor. This could be a pointer to your engine or game object for example. */
        AZ::Entity*             m_entity;               /**< The entity to which the actor instance belongs to. */
        float                   m_boundsUpdateFrequency; /**< The bounds update frequency. Which is a time value in seconds. */
        float                   m_boundsUpdatePassedTime;/**< The time passed since the last bounds update. */
        float                   m_motionSamplingRate;    /**< The motion sampling rate in seconds, where 0.1 would mean to update 10 times per second. A value of 0 or lower means to update every frame. */
        float                   m_motionSamplingTimer;   /**< The time passed since the last time we sampled motions/anim graphs. */
        float                   m_visualizeScale;        /**< Some visualization scale factor when rendering for example normals, to be at a nice size, relative to the character. */
        size_t                  m_lodLevel;              /**< The current LOD level, where 0 is the highest detail. */
        size_t                  m_requestedLODLevel;    /**< Requested LOD level. The actual LOD level will be updated as soon as all transforms for the requested LOD level are ready. */
        uint32                  m_boundsUpdateItemFreq;  /**< The bounds update item counter step size. A value of 1 means every vertex/node, a value of 2 means every second vertex/node, etc. */
        uint32                  m_id;                    /**< The unique identification number for the actor instance. */
        uint32                  m_threadIndex;           /**< The thread index. This specifies the thread number this actor instance is being processed in. */
        EBoundsType             m_boundsUpdateType;      /**< The bounds update type (node based, mesh based or collision mesh based). */
        float m_boundsExpandBy = 0.25f; /**< Expand bounding box by normalized percentage. (Default: 25% greater than the calculated bounding box) */
        uint8                   m_numAttachmentRefs;     /**< Specifies how many actor instances use this actor instance as attachment. */
        uint8                   m_boolFlags;             /**< Boolean flags. */
        uint32_t m_lightingChannelMask = 1;

        /**
         * Boolean masks, as replacement for having several bools as members.
         */
        enum
        {
            BOOL_BOUNDSUPDATEENABLED    = 1 << 0,   /**< Should we automatically update bounds for this node? */
            BOOL_ISVISIBLE              = 1 << 1,   /**< Is this node visible? */
            BOOL_RENDER                 = 1 << 2,   /**< Should this actor instance trigger the OnRender callback method? */
            BOOL_NORMALIZEDMOTIONLOD    = 1 << 3,   /**< Use normalized motion LOD maximum error values? */
            BOOL_USEDFORVISUALIZATION   = 1 << 4,   /**< Indicates if the actor is used for visualization specific things and is not used as a normal in-game actor. */
            BOOL_ENABLED                = 1 << 5,   /**< Exclude the actor instance from the scheduled updates? If so, it is like the actor instance won't exist. */
            BOOL_MOTIONEXTRACTION       = 1 << 6,   /**< Enabled when motion extraction should be active on this actor instance. This still requires the Actor to have a valid motion extraction node setup, and individual motion instances having motion extraction enabled as well. */

#if defined(EMFX_DEVELOPMENT_BUILD)
            BOOL_OWNEDBYRUNTIME         = 1 << 7    /**< Set if the actor instance is used/owned by the engine runtime. */
#endif // EMFX_DEVELOPMENT_BUILD
        };

        /**
         * The constructor.
         * @param actor The actor where this actor instance should be created from.
         * @param threadIndex The thread index to create the instance on.
         */
        ActorInstance(Actor* actor, AZ::Entity* entity, uint32 threadIndex = 0);

        /**
         * The destructor.
         * This automatically unregisters it from the actor manager as well.
         */
        ~ActorInstance();

        /**
         * Increase the attachment reference count.
         * The number of attachment refs represents how many times this actor instance is an attachment.
         * This is only allowed to be one. This is used for debugging mostly, to prevent you from adding the same attachment
         * to multiple actor instances.
         * @param numToIncreaseWith The number to increase the counter with.
         */
        void IncreaseNumAttachmentRefs(uint8 numToIncreaseWith = 1);

        /**
         * Increase the attachment reference count.
         * The number of attachment refs represents how many times this actor instance is an attachment.
         * This is only allowed to be one. This is used for debugging mostly, to prevent you from adding the same attachment
         * to multiple actor instances.
         * @param numToDecreaseWith The number to decrease the counter with.
         */
        void DecreaseNumAttachmentRefs(uint8 numToDecreaseWith = 1);

        /**
         * Get the number of attachment references.
         * This number represents how many times this actor instance itself is an attachment.
         * This value is mainly used for debugging, as you are not allowed to attach the same actor instance to muliple other actor instances.
         * @result The number of times that this attachment is an attachment in another actor instance.
         */
        uint8 GetNumAttachmentRefs() const;

        /**
         * Set the actor instance where we are attached to.
         * Please note that if you want to set an attachment yourself, you have to use the ActorInstance::AddAttachment().
         * That method will handle setting all such values like you set with this method.
         * @param actorInstance The actor instance where we are attached to.
         */
        void SetAttachedTo(ActorInstance* actorInstance);

        /**
         * Set the attachment where this actor instance is part of.
         * So if this actor instance is a gun, and the gun is attached to some cowboy actor instance, then the self attachment object
         * that you specify as parameter here, is the attachment object for the gun that was added to the cowboy actor instance.
         * @param selfAttachment The attachment where this actor instance takes part of.
         */
        void SetSelfAttachment(Attachment* selfAttachment);

        /**
         * Enable boolean flags.
         * @param flag The flags to enable.
         */
        void EnableFlag(uint8 flag);

        /**
         * Disable boolean flags.
         * @param flag The flags to disable.
         */
        void DisableFlag(uint8 flag);

        /**
         * Enable or disable specific flags.
         * @param flag The flags to modify.
         * @param enabled Set to true to enable the flags, or false to disable them.
         */
        void SetFlag(uint8 flag, bool enabled);

        /**
         * Set the skeletal detail level node flags and enable or disable the nodes accordingly.
         * In total there are 32 possible skeletal LOD levels, where 0 is the highest detail, and 31 the lowest detail.
         * Each detail level can disable specific nodes in the actor. These disabled nodes will not be processed at all by EMotion FX.
         * It is important to know that disabled nodes never should be used inside skinning information or on other places where their transformations
         * are needed.
         * @param level The skeletal detail LOD level. Values higher than 31 will be automatically clamped to 31.
         */
        void SetSkeletalLODLevelNodeFlags(size_t level);

        /*
         * Update the LOD level in case a change was requested.
         * This function should only be called from within UpdateTransformations() as we should not change the LOD level while not all transforms used
         * by the LOD are ready. When switching from a lower LOD (less joints used) to a more detailed version of the skeleton, transforms from the
         * newly enabled joints (the ones that were not present and thus also not updated in the lower LOD level)will contain incorrect data.
         */
        void UpdateLODLevel();
    };
}   // namespace EMotionFX
