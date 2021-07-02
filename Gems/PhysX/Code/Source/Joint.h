/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Physics/Joint.h>

namespace AzPhysics
{
    struct SimulatedBody;
}

namespace PhysX
{
    class D6JointLimitConfiguration
        : public Physics::JointLimitConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(D6JointLimitConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(D6JointLimitConfiguration, "{90C5C23D-16C0-4F23-AD50-A190E402388E}", Physics::JointLimitConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        const char* GetTypeName() override;

        float m_swingLimitY = 45.0f; ///< Maximum angle in degrees from the Y axis of the joint frame.
        float m_swingLimitZ = 45.0f; ///< Maximum angle in degrees from the Z axis of the joint frame.
        float m_twistLimitLower = -45.0f; ///< Lower limit in degrees for rotation about the X axis of the joint frame.
        float m_twistLimitUpper = 45.0f; ///< Upper limit in degrees for rotation about the X axis of the joint frame.
    };

    class Joint
        : public Physics::Joint
    {
    public:
        AZ_CLASS_ALLOCATOR(Joint, AZ::SystemAllocator, 0);
        AZ_RTTI(Joint, "{3C739E22-8EF0-419F-966B-C575A1F5A08B}", Physics::Joint);

        Joint(physx::PxJoint* pxJoint, AzPhysics::SimulatedBody* parentBody,
            AzPhysics::SimulatedBody* childBody);

        virtual ~Joint() = default;

        AzPhysics::SimulatedBody* GetParentBody() const override;
        AzPhysics::SimulatedBody* GetChildBody() const override;
        void SetParentBody(AzPhysics::SimulatedBody* parentBody) override;
        void SetChildBody(AzPhysics::SimulatedBody* childBody) override;
        const AZStd::string& GetName() const override;
        void SetName(const AZStd::string& name) override;
        void* GetNativePointer() override;

    protected:
        bool SetPxActors();

        using PxJointUniquePtr = AZStd::unique_ptr<physx::PxJoint, AZStd::function<void(physx::PxJoint*)>>;
        PxJointUniquePtr m_pxJoint;
        AzPhysics::SimulatedBody* m_parentBody;
        AzPhysics::SimulatedBody* m_childBody;
        AZStd::string m_name;
    };

    class D6Joint
        : public Joint
    {
    public:
        AZ_CLASS_ALLOCATOR(D6Joint, AZ::SystemAllocator, 0);
        AZ_RTTI(D6Joint, "{962C4044-2BD2-4E4C-913C-FB8E85A2A12A}", Joint);

        D6Joint(physx::PxJoint* pxJoint, AzPhysics::SimulatedBody* parentBody,
            AzPhysics::SimulatedBody* childBody)
            : Joint(pxJoint, parentBody, childBody)
        {
        }
        virtual ~D6Joint() = default;

        const AZ::Crc32 GetNativeType() const override;
        void GenerateJointLimitVisualizationData(
            float scale,
            AZ::u32 angularSubdivisions,
            AZ::u32 radialSubdivisions,
            AZStd::vector<AZ::Vector3>& vertexBufferOut,
            AZStd::vector<AZ::u32>& indexBufferOut,
            AZStd::vector<AZ::Vector3>& lineBufferOut,
            AZStd::vector<bool>& lineValidityBufferOut) override;
    };

    struct D6JointState
    {
        float m_swingAngleY;
        float m_swingAngleZ;
        float m_twistAngle;
    };

    /// A fixed joint locks 2 bodies relative to one another on all axes of freedom.
    class FixedJoint : public Joint
    {
    public:
        AZ_CLASS_ALLOCATOR(FixedJoint, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(FixedJoint, "{203FB99C-7DC5-478A-A52C-A1F2AAF61FB8}");

        FixedJoint(physx::PxJoint* pxJoint, AzPhysics::SimulatedBody* parentBody,
            AzPhysics::SimulatedBody* childBody)
            : Joint(pxJoint, parentBody, childBody)
        {
        }

        const AZ::Crc32 GetNativeType() const override;
        void GenerateJointLimitVisualizationData(
            float /*scale*/,
            AZ::u32 /*angularSubdivisions*/,
            AZ::u32 /*radialSubdivisions*/,
            AZStd::vector<AZ::Vector3>& /*vertexBufferOut*/,
            AZStd::vector<AZ::u32>& /*indexBufferOut*/,
            AZStd::vector<AZ::Vector3>& /*lineBufferOut*/,
            AZStd::vector<bool>& /*lineValidityBufferOut*/) override {}
    };

    /// A hinge joint locks 2 bodies relative to one another except about the x-axis of the joint between them.
    class HingeJoint : public Joint
    {
    public:
        AZ_CLASS_ALLOCATOR(HingeJoint, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(HingeJoint, "{8EFF1002-B08C-47CE-883C-82F0CF3736E0}");

        HingeJoint(physx::PxJoint* pxJoint, AzPhysics::SimulatedBody* parentBody,
            AzPhysics::SimulatedBody* childBody)
            : Joint(pxJoint, parentBody, childBody)
        {
        }

        const AZ::Crc32 GetNativeType() const override;
        void GenerateJointLimitVisualizationData(
            float /*scale*/,
            AZ::u32 /*angularSubdivisions*/,
            AZ::u32 /*radialSubdivisions*/,
            AZStd::vector<AZ::Vector3>& /*vertexBufferOut*/,
            AZStd::vector<AZ::u32>& /*indexBufferOut*/,
            AZStd::vector<AZ::Vector3>& /*lineBufferOut*/,
            AZStd::vector<bool>& /*lineValidityBufferOut*/) override {}
    };

    /// A ball joint locks 2 bodies relative to one another except about the y and z axes of the joint between them.
    class BallJoint : public Joint
    {
    public:
        AZ_CLASS_ALLOCATOR(BallJoint, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(BallJoint, "{9FADA1C2-0E2F-4E1B-9E83-6292A1606372}");

        BallJoint(physx::PxJoint* pxJoint, AzPhysics::SimulatedBody* parentBody,
            AzPhysics::SimulatedBody* childBody)
            : Joint(pxJoint, parentBody, childBody)
        {
        }

        const AZ::Crc32 GetNativeType() const override;
        void GenerateJointLimitVisualizationData(
            float /*scale*/,
            AZ::u32 /*angularSubdivisions*/,
            AZ::u32 /*radialSubdivisions*/,
            AZStd::vector<AZ::Vector3>& /*vertexBufferOut*/,
            AZStd::vector<AZ::u32>& /*indexBufferOut*/,
            AZStd::vector<AZ::Vector3>& /*lineBufferOut*/,
            AZStd::vector<bool>& /*lineValidityBufferOut*/) override {}
    };

    /// Common parameters for all physics joint types.
    class GenericJointConfiguration
    {
    public:
        enum class GenericJointFlag : AZ::u16
        {
            None = 0,
            Breakable = 1,
            SelfCollide = 1 << 1
        };

        AZ_CLASS_ALLOCATOR(GenericJointConfiguration, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(GenericJointConfiguration, "{AB2E2F92-0248-48A8-9DDD-21284AF0C1DF}");
        static void Reflect(AZ::ReflectContext* context);
        static bool VersionConverter(
            AZ::SerializeContext& context,
            AZ::SerializeContext::DataElementNode& classElement);

        GenericJointConfiguration() = default;
        GenericJointConfiguration(float forceMax,
            float torqueMax,
            AZ::Transform localTransformFromFollower,
            AZ::EntityId leadEntity,
            AZ::EntityId followerEntity,
            GenericJointFlag flags);

        bool GetFlag(GenericJointFlag flag); ///< Returns if a particular flag is set as a bool.

        GenericJointFlag m_flags = GenericJointFlag::None; ///< Flags that indicates if joint is breakable, self-colliding, etc.. Converting joint between breakable/non-breakable at game time is allowed.
        float m_forceMax = 1.0f; ///< Max force joint can tolerate before breaking.
        float m_torqueMax = 1.0f; ///< Max torque joint can tolerate before breaking.
        AZ::EntityId m_leadEntity; ///< EntityID for entity containing body that is lead to this joint constraint.
        AZ::EntityId m_followerEntity; ///< EntityID for entity containing body that is follower to this joint constraint.
        AZ::Transform m_localTransformFromFollower; ///< Joint's location and orientation in the frame (coordinate system) of the follower entity.
    };
    AZ_DEFINE_ENUM_BITWISE_OPERATORS(PhysX::GenericJointConfiguration::GenericJointFlag)

    /// Generic pair of limit values for joint types, e.g. a pair of angular values.
    /// This is different from JointLimitConfiguration used in non-generic joints for character/ragdoll/animation.
    class GenericJointLimitsConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(GenericJointLimitsConfiguration, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(GenericJointLimitsConfiguration, "{9D129B49-F4E6-4F2A-B94D-AC2D6AC6CE02}");
        static void Reflect(AZ::ReflectContext* context);

        GenericJointLimitsConfiguration() = default;
        GenericJointLimitsConfiguration(float damping
            , bool isLimited
            , bool isSoftLimit
            , float limitFirst
            , float limitSecond
            , float stiffness
            , float tolerance);

        bool m_isLimited = true; ///< Specifies if limits are applied to the joint constraints. E.g. if the swing angles are limited.
        bool m_isSoftLimit = false; ///< If limit is soft, spring and damping are used, otherwise tolerance is used. Converting between soft/hard limit at game time is allowed.
        float m_damping = 20.0f; ///< The damping strength of the drive, the force proportional to the velocity error. Used if limit is soft.
        float m_limitFirst = 45.0f; ///< Positive angle limit in the case of twist angle limits, Y-axis swing limit in the case of cone limits.
        float m_limitSecond = 45.0f; ///< Negative angle limit in the case of twist angle limits, Z-axis swing limit in the case of cone limits.
        float m_stiffness = 100.0f; ///< The spring strength of the drive, the force proportional to the position error. Used if limit is soft.
        float m_tolerance = 0.1f; ///< Distance from the joint at which limits becomes enforced. Used if limit is hard.
    };

    class JointUtils
    {
    public:
        static AZStd::vector<AZ::TypeId> GetSupportedJointTypes();

        static AZStd::shared_ptr<Physics::JointLimitConfiguration> CreateJointLimitConfiguration(AZ::TypeId jointType);

        static AZStd::shared_ptr<Physics::Joint> CreateJoint(const AZStd::shared_ptr<Physics::JointLimitConfiguration>& configuration,
            AzPhysics::SimulatedBody* parentBody, AzPhysics::SimulatedBody* childBody);

        static D6JointState CalculateD6JointState(
            const AZ::Quaternion& parentWorldRotation,
            const AZ::Quaternion& parentLocalRotation,
            const AZ::Quaternion& childWorldRotation,
            const AZ::Quaternion& childLocalRotation);

        static bool IsD6SwingValid(
            float swingAngleY,
            float swingAngleZ,
            float swingLimitY,
            float swingLimitZ);

        static void AppendD6SwingConeToLineBuffer(
            const AZ::Quaternion& parentLocalRotation,
            float swingAngleY,
            float swingAngleZ,
            float swingLimitY,
            float swingLimitZ,
            float scale,
            AZ::u32 angularSubdivisions,
            AZ::u32 radialSubdivisions,
            AZStd::vector<AZ::Vector3>& lineBufferOut,
            AZStd::vector<bool>& lineValidityBufferOut);

        static void AppendD6TwistArcToLineBuffer(
            const AZ::Quaternion& parentLocalRotation,
            float twistAngle,
            float twistLimitLower,
            float twistLimitUpper,
            float scale,
            AZ::u32 angularSubdivisions,
            AZ::u32 radialSubdivisions,
            AZStd::vector<AZ::Vector3>& lineBufferOut,
            AZStd::vector<bool>& lineValidityBufferOut);

        static void AppendD6CurrentTwistToLineBuffer(
            const AZ::Quaternion& parentLocalRotation,
            float twistAngle,
            float twistLimitLower,
            float twistLimitUpper,
            float scale,
            AZStd::vector<AZ::Vector3>& lineBufferOut,
            AZStd::vector<bool>& lineValidityBufferOut);

        static void GenerateJointLimitVisualizationData(
            const Physics::JointLimitConfiguration& configuration,
            const AZ::Quaternion& parentRotation,
            const AZ::Quaternion& childRotation,
            float scale,
            AZ::u32 angularSubdivisions,
            AZ::u32 radialSubdivisions,
            AZStd::vector<AZ::Vector3>& vertexBufferOut,
            AZStd::vector<AZ::u32>& indexBufferOut,
            AZStd::vector<AZ::Vector3>& lineBufferOut,
            AZStd::vector<bool>& lineValidityBufferOut);

        static AZStd::unique_ptr<Physics::JointLimitConfiguration> ComputeInitialJointLimitConfiguration(
            const AZ::TypeId& jointLimitTypeId,
            const AZ::Quaternion& parentWorldRotation,
            const AZ::Quaternion& childWorldRotation,
            const AZ::Vector3& axis,
            const AZStd::vector<AZ::Quaternion>& exampleLocalRotations);
    };
} // namespace PhysX
