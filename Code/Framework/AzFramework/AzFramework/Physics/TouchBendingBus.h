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
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Quaternion.h>

namespace Physics
{
    constexpr const char * const AZ_TOUCH_BENDING_WINDOW = "AzTouchBending";

    // Bone orientation
    //
    //    _  TOP Point            Z+ up
    //   | |                      ^
    //   | |                      |
    //   |*|  (0,0,0)   X- <------|--------> X+
    //   | |                      |
    //   |_| BOTTOM Point         |
    //                            Z-

    /// SpinePoint contains properties of mass, thickness, damping and stiffness
    /// for the bone that will be made. The SpinePoint is the BOTTOM point
    /// of a Bone.
    struct SpinePoint
    {
        ///mass in Kg.
        float m_mass;

        ///If you imagine the Bone to be a cylinder, this is its radius in meters.
        float m_thickness; 

        ///A value from 0.0 to 1.0. 0.0 means no damping, lots of back and forth movement around its original pose.
        ///1.0 means maximum damping, the segment will quickly converge back to its original pose.
        float m_damping; 
        
        ///A value from 0.0 to 1.0. 0.0 means no stiffness, the segment will look like a sad willow,
        ///It would never return to its original pose.
        float m_stiffness;

        ///Position is in Model Space.
        AZ::Vector3 m_position;
    };

    struct Spine
    {
        ///Index of parent spine. -1 if no parent.
        int m_parentSpineIndex;

        ///Index of the point within the parent spine array of segments.
        ///-1 if no parent.
        int m_parentPointIndex;

        ///Array of segments.
        AZStd::vector<SpinePoint> m_points;
    };

    typedef void* SpineTreeIDType;

    ///SpineTree is an archetype. This is basically the AzFramework version
    ///of CStatObj.SSpine.
    struct SpineTree
    {
        ///Unique Identifier Of this SpineTree.
        SpineTreeIDType m_spineTreeId;

        ///A SpineTree ALWAYS contains at least one spine.
        AZStd::vector<Spine> m_spines;

        ///Helper method.
        size_t CalculateTotalNumberOfBones() const
        {
            size_t numberOfBones = 0;
            for (const Spine& spine : m_spines)
            {
                numberOfBones += spine.m_points.size() - 1;
            }
            return numberOfBones;
        }
    };

    ///The Engine side of Touch bending uses this as an opaque handle.
    ///Only the TouchBending Gem knows what's inside.
    ///This handle corresponds one-to-one with a unique Vegetation Render Node instance.
    struct TouchBendingTriggerHandle;

    ///The Engine side of Touch bending uses this as an opaque handle.
    ///Only the TouchBending Gem knows what's inside.
    ///This handle corresponds one-to-one with a unique CStatObjFoliage instance.
    struct TouchBendingSkeletonHandle;

    ///Used by TouchBending Gem to talk back with the Engine.
    class ITouchBendingCallback
    {
    public:
        ITouchBendingCallback() = default;
        virtual ~ITouchBendingCallback() = default;

        /** @brief Checks if a render node is within e_CullVegActivation radius from the camera 
         *
         *  @param privateData Pointer to the Render Node inside the Engine that represents
         *         the touch bendable entity. From the point of view of the TouchBending Gem this
         *         is an opaque pointer, but from the point of view of the engine this is a
         *         CVegetation render node.
         *  @returns Returns a non-zero SpineTreeIDType if the Render Node is within
         *           e_CullVegActivation radius from the center of the main camera.
         *           Otherwise returns zero.
         */
        virtual SpineTreeIDType CheckDistanceToCamera(const void* privateData) = 0;
    
        /** @brief Builds a SpineTree archetype object using its SpineTreeIDType.
         *
         *  \p privateData is a CVegetation*
         *  \p spineTreeId is a CStatObj*
         *
         *  @param privateData Pointer to the Render Node inside the Engine that represents
         *         the touch bendable entity.
         *  @param spineTreeId Spine Tree Archetype Identifier as given previously by the Engine.
         *  @param spineTreeOut Output SpineTree archetype object.
         *  @returns TRUE if such \p spineTreeId is valid and a SpineTree archetype was successfully built.
         *           Otherwise returns FALSE.
         */
        virtual bool BuildSpineTree(const void* privateData, SpineTreeIDType spineTreeId, SpineTree& spineTreeOut) = 0;

        /** TouchBending Gem calls this to notify the Engine that a unique PhysicalizedSkeleton instance was built
         *  on behalf of \p privateData.
         *
         *  The Engine uses this event to build a CStatObjFoliage to keep track of active touch bendable objects.
         *  The Engine will keep CStatObjFoliage alive as long as it is touched or for a specific lifetime in seconds
         *  defined by the CVar e_FoliageBranchesTimeout.
         *
         *  @param privateData Pointer to the Render Node inside the Engine that represents
         *         the touch bendable entity.
         *  @param skeletonHandle Opaque pointer that the CStatObjFoliage must keep a copy to. The engine
         *         should should use it later when calling *Skeleton*() named methods of the TouchBendingBus.
         *  @returns true if the CStatObjFoliage was created successfully. It may return false only for cases where the CStatObj
         *           was removed and CStatObjFoliage can only be created if CStatObj is not null. 
         */
        virtual bool OnPhysicalizedTouchBendingSkeleton(const void* privateData, TouchBendingSkeletonHandle* skeletonHandle) = 0;
    }; //class ITouchBendingCallback


    //Exact same memory format as QuatTS
    //CStatObjFoliage::ComputeSkinningTransformations() uses:
    //QuatTS.q[x,y,z] as TOP joint position.
    //QuatTS.t[x,y,z] as BOTTOM  joint position.
    //QuatTS.s CStatObjFoliage::GetSkinningData() reads this value for the first bone of each spine
    //         as marker for valid data, if less than zero, the spine is skipped by the Skinning code.
    // A bone has two joints, TOP and BOTTOM:
    //
    //    _  TOP                  Z+ up
    //   | |                      ^
    //   | |                      |
    //   |*|  (0,0,0)   X- <------|--------> X+
    //   | |                      |
    //   |_| BOTTOM               |
    //                            Z-
    struct JointPositions
    {
        float m_TopJointLocation[3];    //Equivalent to QuatTS.q.xyz (ijk)
        float m_qw;                     //Equivalent to QuatTS.q.w
        float m_BottomJointLocation[3]; //Equivalent to QuatTS.t
        float m_hasNewData;             //Equivalent to QuatTS.s (See description above about  CStatObjFoliage::GetSkinningData()).
    };

    /**
     * Replacement of CryPhysics Touch Bending simulation.
     */
    class TouchBendingRequest
        : public AZ::EBusTraits
    {
    public:
        AZ_RTTI(TouchBendingRequest, "{4E9DE1BE-F0C7-47E7-B315-9302F62D044C}");

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual ~TouchBendingRequest() = default;

        /// If the EBUS implementation (aka TouchBending Gem) returns TRUE
        /// all of the Physics simulation for Touch Bendable CVegetation is done
        /// by the TouchBending Gem with PhysX. If it returns FALSE the engine
        /// will default to CryPhysics.
        virtual bool IsTouchBendingEnabled() const = 0;
        
        /** @brief Creates a TouchBending Trigger with a simple trigger box.
         *
         *  Initially a TouchBending Trigger is nothing more than a trigger volume. There's
         *  no skeleton, etc. It will serve as the trigger, that when touched, builds a unique physicalized
         *  skeleton with the same amount of bones as a SpineTree. Recall that SpineTree is an archetype.
         *  The TouchBending Gem Builds a TouchBendingSkeletonHandle based on a SpineTree when TouchBendingTriggerHandle is touched.
         *
         *  When the User adds an object via the Vegetation panel of the "Terrain Tool" UI
         *  this method will be called by the engine.
         *
         *  If the User has enabled the Dynamic Vegetation Gem this method can be called
         *  at runtime as CVegetation nodes appear within the Camera Frustum.
         *
         *  @param worldTransform This transform includes the scale factor. It is the position of the root
         *         of the CVegetation node.
         *  @param worldAabb Axis Aligned Bounding Box in world coordinates of the CVegetation node.
         *  @param callback The engine gives this callback to the TouchBending Gem for further communication.
         *  @param callbackPrivateData The Engine gives this opaque handle to TouchBending Gem so the Gem it can properly address it
         *         address the right Render Node instance when using the \p callback.
         *  @returns An opaque handle of a TouchBending Trigger Instance created by the TouchBending Gem.
         */
        virtual TouchBendingTriggerHandle* CreateTouchBendingTrigger(const AZ::Transform& worldTransform,
            const AZ::Aabb&worldAabb, ITouchBendingCallback* callback, const void * callbackPrivateData) = 0;

        /** @brief Used by the engine to notify TouchBending Gem about the visibility status of the physicalized skeleton.
         *
         *  @param skeletonHandle Opaque pointer to the physicalized skeleton created by TouchBending Gem.
         *  @param isVisible if TRUE the engine finds out that the skeleton is visible. If FALSE the engine calculated
         *         that the skeleton is either totally outside of the Camera Frustum or its distance
         *         from the camera exceeds the CVAR e_CullVegActivation.
         *  @param skeletonBoneCountOut It is the responsibility of the TouchBending Gem to fill this out
         *         with the number of bones available for skinning.
         *  @param triggerTouchCountOut It is the responsibility of the TouchBending Gem to fill this out
         *         with the number of objects that are touching the touch bending trigger.
         *  @returns void
         */
        virtual void SetTouchBendingSkeletonVisibility(Physics::TouchBendingSkeletonHandle* skeletonHandle,
            bool isVisible, AZ::u32& skeletonBoneCountOut, AZ::u32& triggerTouchCountOut) = 0;

        /** @brief The engine calls this when it is deleting the Render Node.
         *
         *  When the User deletes an object via the Vegetation panel of the Rollup Bar (Legacy) UI
         *  this method will be called by the engine.
         *
         *  If the User has enabled the Dynamic Vegetation Gem this method can be called
         *  at runtime as CVegetation nodes disappear from the Camera Frustum.
         *
         *  @param handle Opaque handle of the TouchBending trigger instance as created by the
         *         TouchBending Gem.
         *  @returns void
         */
        virtual void DeleteTouchBendingTrigger(TouchBendingTriggerHandle* handle) = 0;


        /** @brief The engine calls this to destroy a physicalized skeleton.
         *
         *  The touch bending trigger remains active.
         *  This means that in the future something may touch the trigger
         *  and the skeleton is created again.
         *
         *  @param skeletonHandle Opaque handle of the TouchBending Skeleton as created by the
         *         TouchBending Gem. The skeleton will be removed from the Physics World.
         *  @returns
         */
        virtual void DephysicalizeTouchBendingSkeleton(TouchBendingSkeletonHandle* skeletonHandle) = 0;


        /** Reads the current position of the pair-of-joints per bone of the Skeleton into the \p jointPositions
         *  buffer. 
         * 
         *  @param skeletonHandle Opaque handle of the physicalized skeleton instance as created by the
         *         TouchBending Gem. 
         *  @param jointPositions Buffer where the Top and Bottom Joint positions for each bone
         *         is written to. Please read the documentation of "struct JointPositions" for clarification.
         *  @returns void
         */
        virtual void ReadJointPositionsOfSkeleton(TouchBendingSkeletonHandle* skeletonHandle, JointPositions* jointPositions) = 0;
    };
    using TouchBendingBus = AZ::EBus<TouchBendingRequest>;

    /// A helper method to test if there's a Gem implementing the TouchBendingBus.
    AZ_INLINE bool IsTouchBendingEnabled()
    {
        bool isEnabled = false;
        TouchBendingBus::BroadcastResult(isEnabled, &TouchBendingBus::Events::IsTouchBendingEnabled);
        return isEnabled;
    }
}
