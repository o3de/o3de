/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
} // namespace AZ

namespace EMotionFX
{
    class Actor;
    class SimulatedObject;
    class SimulatedObjectSetup;

    class SimulatedCommon
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(SimulatedCommon, "{CAABEF38-EBE6-4C39-B579-88228CE85B35}");

        virtual ~SimulatedCommon() = default;
    };

    class SimulatedJoint
        : public SimulatedCommon
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(SimulatedJoint, "{4434F175-2A60-4F54-9A7D-243DAAD8C811}", SimulatedCommon);

        enum class AutoExcludeMode : AZ::u8
        {
            None = 0,
            Self = 1,
            SelfAndNeighbors = 2,
            All = 3
        };

        SimulatedJoint() = default;
        SimulatedJoint(SimulatedObject* object, AZ::u32 skeletonJointIndex);
        ~SimulatedJoint() override = default;

        SimulatedJoint* FindParentSimulatedJoint() const;
        SimulatedJoint* FindChildSimulatedJoint(size_t childIndex) const;
        AZ::Outcome<size_t> CalculateSimulatedJointIndex() const;
        size_t CalculateNumChildSimulatedJoints() const;
        size_t CalculateNumChildSimulatedJointsRecursive() const;
        AZ::u32 CalculateChildIndex() const;

        bool InitAfterLoading(SimulatedObject* object);

        void SetSimulatedObject(SimulatedObject* object) { m_object = object; }
        void SetSkeletonJointIndex(AZ::u32 jointIndex) { m_jointIndex = jointIndex; }
        void SetConeAngleLimit(float degrees) { m_coneAngleLimit = degrees; }
        void SetMass(float mass) { m_mass = mass; }
        void SetCollisionRadius(float radius)
        {
            AZ_Assert(radius >= 0.0f, "Expecting simulated joint collision radius to be greater or equal to zero.");
            m_radius = radius;
        }
        void SetStiffness(float stiffness) { m_stiffness = stiffness; }
        void SetDamping(float damping) { m_damping = damping; }
        void SetGravityFactor(float factor) { m_gravityFactor = factor; }
        void SetFriction(float friction) { m_friction = friction; }
        void SetPinned(bool pinned) { m_pinned = pinned; }
        void SetColliderExclusionTags(const AZStd::vector<AZStd::string>& exclusionTagList) { m_colliderExclusionTags = exclusionTagList; }
        void SetAutoExcludeMode(AutoExcludeMode mode) { m_autoExcludeMode = mode; }
        void SetGeometricAutoExclusion(bool enabled) { m_autoExcludeGeometric = enabled; }

        SimulatedObject* GetSimulatedObject() const { return m_object; }
        AZ::u32 GetSkeletonJointIndex() const { return m_jointIndex; }
        float GetConeAngleLimit() const { return m_coneAngleLimit; }
        float GetMass() const { return m_mass; }
        float GetCollisionRadius() const { return m_radius; }
        float GetStiffness() const { return m_stiffness; }
        float GetDamping() const { return m_damping; }
        float GetGravityFactor() const { return m_gravityFactor; }
        float GetFriction() const { return m_friction; }
        const AZStd::vector<AZStd::string>& GetColliderExclusionTags() const { return m_colliderExclusionTags; }
        bool IsPinned() const { return m_pinned; }
        bool IsRootJoint() const;
        bool IsGeometricAutoExclusion() const { return m_autoExcludeGeometric; }
        AutoExcludeMode GetAutoExcludeMode() const { return m_autoExcludeMode; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZ::Crc32 GetPinnedOptionVisibility() const;

        SimulatedObject* m_object = nullptr; /**< The simulated object we belong to. */
        AZ::u32 m_jointIndex = 0; /**< The joint index inside the skeleton of the actor. */
        AZStd::string m_jointName; /**< The joint name in the actor skeleton. */
        float m_coneAngleLimit = 60.0f; /**< The conic angular limit, in degrees. A value of 180 means there are no limits. */
        float m_mass = 1.0f; /**< The mass of the joint. */
        float m_radius = 0.025f; /**< The collision radius. */
        float m_stiffness = 0.0f; /**< The stiffness of the bone. A value of 0.0 means no stiffness at all. A value of say 50 would make it move back into its original pose. Making things look bouncy. */
        float m_damping = 0.001f; /**< The damping value, which defaults on 0.001f. A value of 0 would mean the bone would oscilate forever. Higher values make it come to rest sooner. */
        float m_gravityFactor = 1.0f; /**< The factor with which the gravity force will be multiplied with. Default is 1.0. A value of 2.0 means two times the amount of gravity being applied to this particle. */
        float m_friction = 0.0f; /**< The friction factor, between 0 and 1, which happens when a collision happens with a surface. */
        AZStd::vector<AZStd::string> m_colliderExclusionTags; /**< The list of collider tags that we would like to exclude in collision detection for this joint. */
        bool m_pinned = false; /**< Are we pinned or not? Pinned joints follow the original joint, so in a way they are pinned to a given skeletal joint. */
        bool m_autoExcludeGeometric = false; /**< Geometric auto exclusion? When enabled this checks whether the joint is actually inside the collider or not. */
        AutoExcludeMode m_autoExcludeMode = AutoExcludeMode::Self; /**< The auto exclusion mode for collider self-collision prevention. */
    };

    class SimulatedObject
        : public SimulatedCommon
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(SimulatedObject, "{8CF0F474-69DC-4DE3-AF19-002F19DA27DB}", SimulatedCommon);

        SimulatedObject() = default;
        explicit SimulatedObject(SimulatedObjectSetup* setup, const AZStd::string& objectName = {});
        ~SimulatedObject() override;

        void Clear();

        SimulatedJoint* FindSimulatedJointBySkeletonJointIndex(AZ::u32 skeletonJointIndex) const;
        bool ContainsSimulatedJoint(const SimulatedJoint* joint) const;
        SimulatedJoint* AddSimulatedJoint(AZ::u32 jointIndex);
        void AddSimulatedJoints(AZStd::vector<AZ::u32> jointIndexes);
        void AddSimulatedJointAndChildren(AZ::u32 jointIndex);
        void RemoveSimulatedJoint(AZ::u32 jointIndex, bool removeChildren = false);
        size_t GetNumSimulatedJoints() const { return m_joints.size(); }

        SimulatedJoint* GetSimulatedRootJoint(size_t rootIndex) const;
        size_t GetNumSimulatedRootJoints() const;
        size_t GetSimulatedRootJointIndex(const SimulatedJoint* rootJoint) const;

        const AZStd::vector<SimulatedJoint*>& GetSimulatedJoints() const { return m_joints; }
        const SimulatedObjectSetup* GetSimulatedObjectSetup() const { return m_simulatedObjectSetup; }
        SimulatedJoint* GetSimulatedJoint(size_t index) const { return m_joints[index]; }

        void InitAfterLoading(SimulatedObjectSetup* setup);
        static void Reflect(AZ::ReflectContext* context);

        const AZStd::vector<AZStd::string>& GetColliderTags() const;
        void SetColliderTags(const AZStd::vector<AZStd::string>& tags);

        const AZStd::string& GetName() const;
        void SetName(const AZStd::string& newName);

        float GetGravityFactor() const;
        void SetGravityFactor(float newGravityFactor);

        float GetStiffnessFactor() const;
        void SetStiffnessFactor(float newStiffnessFactor);

        float GetDampingFactor() const;
        void SetDampingFactor(float newDampingFactor);

    private:
        void SetSimulatedObjectSetup(SimulatedObjectSetup* setup) { m_simulatedObjectSetup = setup; }
        void BuildRootJointList();
        void SortJointList();
        void MergeAndMakeJoints(const AZStd::vector<AZ::u32>& jointsToAdd);

        AZStd::string GetJointsTextOverride() const;
        AZStd::string GetColliderTag(int index) const;

        AZStd::vector<SimulatedJoint*> m_joints;
        AZStd::vector<SimulatedJoint*> m_rootJoints; /* The list of joints that are children to the object. ( Grandchildren not counted. ) */
        AZStd::vector<AZStd::string> m_colliderTags;
        AZStd::string m_objectName;
        SimulatedObjectSetup* m_simulatedObjectSetup = nullptr;
        float m_gravityFactor = 1.0f;
        float m_stiffnessFactor = 1.0f;
        float m_dampingFactor = 1.0f;
    };

    class SimulatedObjectSetup
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(SimulatedObjectSetup, "{9FB39BF8-01B4-4CD7-83C1-A5AC9F6B1648}");

        SimulatedObjectSetup() = default;
        explicit SimulatedObjectSetup(Actor* actor);
        virtual ~SimulatedObjectSetup();

        const Actor* GetActor() const { return m_actor; }

        SimulatedObject* AddSimulatedObject(const AZStd::string& objectName = {});
        SimulatedObject* InsertSimulatedObjectAt(size_t index);
        void RemoveSimulatedObject(size_t objectIndex);

        size_t GetNumSimulatedObjects() const { return m_simulatedObjects.size(); }
        const AZStd::vector<SimulatedObject*>& GetSimulatedObjects() const { return m_simulatedObjects; }
        SimulatedObject* GetSimulatedObject(size_t index) const;
        SimulatedObject* FindSimulatedObjectByJoint(const SimulatedJoint* joint) const;
        SimulatedObject* FindSimulatedObjectByName(const char* name) const;
        bool IsSimulatedObjectNameUnique(const AZStd::string& newNameCandidate, const SimulatedObject* checkedSimulatedObject) const;
        AZ::Outcome<size_t> FindSimulatedObjectIndex(const SimulatedObject* object) const;

        void InitAfterLoad(Actor* actor);
        AZStd::shared_ptr<SimulatedObjectSetup> Clone(Actor* newActor) const;
        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::vector<SimulatedObject*> m_simulatedObjects;
        Actor* m_actor = nullptr;
    };
} // namespace EMotionFX
