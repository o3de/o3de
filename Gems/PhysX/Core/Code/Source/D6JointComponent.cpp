/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <Joint/PhysXJointUtils.h>
#include <PhysX/Joint/Configuration/PhysXJointConfiguration.h>
#include <PhysX/MathConversion.h>
#include <PhysX/PhysXLocks.h>
#include <Source/D6JointComponent.h>

namespace PhysX
{
    namespace
    {
        //! Convert D6JointDrive to PxD6JointDrive
        physx::PxD6JointDrive ConvertD6JointDriveToPxD6Drive(const D6JointDrive& d6JointDrive)
        {
            const physx::PxReal stiffness = d6JointDrive.m_stiffness;
            const physx::PxReal damping = d6JointDrive.m_damping;
            const physx::PxReal forceLimit = d6JointDrive.m_forceLimit;
            const bool isAcceleration =
                (d6JointDrive.m_driveFlag == D6JointDriveFlag::Acceleration); // Currently not exposed in the configuration

            return physx::PxD6JointDrive(stiffness, damping, forceLimit, isAcceleration);
        }

        const AZStd::unordered_map<int, physx::PxD6Axis::Enum> idToPxD6AxisEnum = {
            { 0, physx::PxD6Axis::eX },     { 1, physx::PxD6Axis::eY },      { 2, physx::PxD6Axis::eZ },
            { 3, physx::PxD6Axis::eTWIST }, { 4, physx::PxD6Axis::eSWING1 }, { 5, physx::PxD6Axis::eSWING2 }
        };

        const AZStd::unordered_map<int, physx::PxD6Drive::Enum> idToPxD6DriveEnum = {
            { 0, physx::PxD6Drive::eX },     { 1, physx::PxD6Drive::eY },     { 2, physx::PxD6Drive::eZ },
            { 3, physx::PxD6Drive::eTWIST }, { 4, physx::PxD6Drive::eSWING }, { 5, physx::PxD6Drive::eSLERP }
        };

    } // namespace

    void D6JointDrive::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<D6JointDrive>()
                ->Version(1)
                ->Field("DriveType", &D6JointDrive::m_driveType)
                ->Field("Stiffness", &D6JointDrive::m_stiffness)
                ->Field("Damping", &D6JointDrive::m_damping)
                ->Field("ForceLimit", &D6JointDrive::m_forceLimit)
                ->Field("DriveFlag", &D6JointDrive::m_driveFlag);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Enum<D6JointDriveType>("D6JointDriveType", "D6 Joint Drive Type")
                    ->Value("None", D6JointDriveType::None)
                    ->Value("Position", D6JointDriveType::Position)
                    ->Value("Velocity", D6JointDriveType::Velocity);

                editContext->Enum<D6JointDriveFlag>("D6JointDriveFlag", "D6 Joint Drive Flag")
                    ->Value("Force", D6JointDriveFlag::Force)
                    ->Value("Acceleration", D6JointDriveFlag::Acceleration);

                editContext->Class<D6JointDrive>("D6 Joint Drive", "Drive configuration for a D6 joint axis")
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox,
                        &D6JointDrive::m_driveType,
                        "Drive Type",
                        "Type of drive (None, Position, Velocity)")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<D6JointDriveType>())
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &D6JointDrive::m_stiffness, "Stiffness", "Spring stiffness")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &D6JointDrive::IsActive)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &D6JointDrive::m_damping, "Damping", "Damping coefficient")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &D6JointDrive::IsActive)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &D6JointDrive::m_forceLimit, "Force Limit", "Maximum force/torque limit")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &D6JointDrive::IsActive)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &D6JointDrive::m_driveFlag, "Drive Flag", "Drive flag")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<D6JointDriveFlag>())
                    ->Attribute(AZ::Edit::Attributes::Visibility, &D6JointDrive::IsActive);
            }
        }
    }

    void D6JointComponentConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<D6JointComponentConfiguration>()
                ->Version(1)
                ->Field("MotionX", &D6JointComponentConfiguration::m_motionX)
                ->Field("MotionY", &D6JointComponentConfiguration::m_motionY)
                ->Field("MotionZ", &D6JointComponentConfiguration::m_motionZ)
                ->Field("MotionTwist", &D6JointComponentConfiguration::m_motionTwist)
                ->Field("MotionSwing1", &D6JointComponentConfiguration::m_motionSwing1)
                ->Field("MotionSwing2", &D6JointComponentConfiguration::m_motionSwing2)
                ->Field("LimitsX", &D6JointComponentConfiguration::m_motionLimitsX)
                ->Field("LimitsY", &D6JointComponentConfiguration::m_motionLimitsY)
                ->Field("LimitsZ", &D6JointComponentConfiguration::m_motionLimitsZ)
                ->Field("LimitsTwist", &D6JointComponentConfiguration::m_motionLimitsTwist)
                ->Field("LimitsCone", &D6JointComponentConfiguration::m_motionLimitsCone)
                ->Field("DriveX", &D6JointComponentConfiguration::m_driveX)
                ->Field("DriveY", &D6JointComponentConfiguration::m_driveY)
                ->Field("DriveZ", &D6JointComponentConfiguration::m_driveZ)
                ->Field("DriveTwist", &D6JointComponentConfiguration::m_driveTwist)
                ->Field("DriveSwing", &D6JointComponentConfiguration::m_driveSwing)
                ->Field("DriveSlerp", &D6JointComponentConfiguration::m_driveSlerp);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Enum<D6JointAxis>("D6JointAxis", "D6JointAxisLimit")
                    ->Value("Free", D6JointAxis::Free)
                    ->Value("Limited", D6JointAxis::Limited)
                    ->Value("Locked", D6JointAxis::Locked);

                editContext->Class<D6JointComponentConfiguration>("D6 Joint Configuration", "Configuration for D6 joint motion")
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &D6JointComponentConfiguration::m_motionX, "Motion X", "Linear motion along X axis")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<D6JointAxis>())
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &D6JointComponentConfiguration::m_motionLimitsX,
                        "X Limits",
                        "Linear limits along X axis")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &D6JointComponentConfiguration::IsXLimited)

                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &D6JointComponentConfiguration::m_motionY, "Motion Y", "Linear motion along Y axis")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<D6JointAxis>())
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &D6JointComponentConfiguration::m_motionLimitsY,
                        "Y Limits",
                        "Linear limits along Y axis")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &D6JointComponentConfiguration::IsYLimited)

                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &D6JointComponentConfiguration::m_motionZ, "Motion Z", "Linear motion along Z axis")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<D6JointAxis>())
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &D6JointComponentConfiguration::m_motionLimitsZ,
                        "Z Limits",
                        "Linear limits along Z axis")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &D6JointComponentConfiguration::IsZLimited)

                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox,
                        &D6JointComponentConfiguration::m_motionTwist,
                        "Motion Twist",
                        "Angular motion around X axis (twist)")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<D6JointAxis>())
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &D6JointComponentConfiguration::m_motionLimitsTwist,
                        "Twist Limits",
                        "Angular limits for twist (radians)")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &D6JointComponentConfiguration::IsTwistLimited)

                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox,
                        &D6JointComponentConfiguration::m_motionSwing1,
                        "Motion Swing 1",
                        "Angular motion around Y axis")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<D6JointAxis>())
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox,
                        &D6JointComponentConfiguration::m_motionSwing2,
                        "Motion Swing 2",
                        "Angular motion around Z axis")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<D6JointAxis>())
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &D6JointComponentConfiguration::m_motionLimitsCone,
                        "Cone Limits",
                        "Angular cone limits for swing (radians)")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &D6JointComponentConfiguration::IsSwing1Limited)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Drives")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &D6JointComponentConfiguration::m_driveX,
                        "Drive X",
                        "Drive configuration for X axis")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &D6JointComponentConfiguration::m_driveY,
                        "Drive Y",
                        "Drive configuration for Y axis")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &D6JointComponentConfiguration::m_driveZ,
                        "Drive Z",
                        "Drive configuration for Z axis")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &D6JointComponentConfiguration::m_driveTwist,
                        "Drive Twist",
                        "Drive configuration for twist (X rotation)")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &D6JointComponentConfiguration::m_driveSwing,
                        "Drive Swing",
                        "Drive configuration for swing (Y/Z rotation)")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &D6JointComponentConfiguration::m_driveSlerp,
                        "Drive Slerp",
                        "Drive configuration for spherical interpolation");
            }
        }
    }

    void D6JointComponent::Reflect(AZ::ReflectContext* context)
    {
        D6JointDrive::Reflect(context);
        D6JointComponentConfiguration::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<D6JointComponent, JointComponent>()->Version(1)->Field(
                "D6 Configuration", &D6JointComponent::m_d6Configuration);
        }
    }

    D6JointComponent::D6JointComponent(
        const JointComponentConfiguration& configuration,
        const JointGenericProperties& genericProperties,
        const D6JointComponentConfiguration& d6Configuration)
        : JointComponent(configuration, genericProperties)
        , m_d6Configuration(d6Configuration)
    {
    }

    physx::PxD6Motion::Enum ConvertD6JointAxisToPxD6Motion(D6JointAxis axis)
    {
        switch (axis)
        {
        case D6JointAxis::Free:
            return physx::PxD6Motion::eFREE;
        case D6JointAxis::Limited:
            return physx::PxD6Motion::eLIMITED;
        case D6JointAxis::Locked:
            return physx::PxD6Motion::eLOCKED;
        default:
            AZ_Assert(false, "Unsupported D6 joint axis type");
            return physx::PxD6Motion::eLOCKED;
        }
    }

    void D6JointComponent::InitNativeJoint()
    {
        JointComponent::LeadFollowerInfo leadFollowerInfo;
        ObtainLeadFollowerInfo(leadFollowerInfo);
        if (leadFollowerInfo.m_followerActor == nullptr || leadFollowerInfo.m_followerBody == nullptr)
        {
            return;
        }
        // if there is no lead body, this will be a constraint of the follower's global position, so use invalid body handle.
        AzPhysics::SimulatedBodyHandle parentHandle = AzPhysics::InvalidSimulatedBodyHandle;
        if (leadFollowerInfo.m_leadBody != nullptr)
        {
            parentHandle = leadFollowerInfo.m_leadBody->m_bodyHandle;
        }
        else
        {
            AZ_Warning(
                "PhysX",
                false,
                "Entity [%s] Hinge Joint component missing lead entity. This joint will be a global constraint on the follower's global "
                "position.",
                GetEntity()->GetName().c_str());
        }
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        AZ_Assert(sceneInterface, "No sceneInterface");
        AZ_Warning(
            "D6JointComponent", leadFollowerInfo.m_followerActor, "No valid followe actor for D6 joint, D6 joint will not be created.");
        AZ_Warning("D6JointComponent", leadFollowerInfo.m_leadActor, "No valid lead actor for D6 joint, D6 joint will not be created.");

        if (leadFollowerInfo.m_followerActor == nullptr || leadFollowerInfo.m_leadActor == nullptr)
        {
            return;
        }
        PhysX::D6JointLimitConfiguration configuration;
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            m_jointSceneOwner = leadFollowerInfo.m_followerBody->m_sceneOwner;
            m_jointHandle = sceneInterface->AddJoint(
                leadFollowerInfo.m_followerBody->m_sceneOwner, &configuration, parentHandle, leadFollowerInfo.m_followerBody->m_bodyHandle);
            // get native joint
            AZ_Assert(m_jointHandle != AzPhysics::InvalidJointHandle, "Failed to create PhysX D6 joint");
            const auto* joint = sceneInterface->GetJointFromHandle(m_jointSceneOwner, m_jointHandle);
            AZ_Assert(joint, "Failed to get PhysX joint from handle");
            physx::PxJoint* native = static_cast<physx::PxJoint*>(joint->GetNativePointer());
            m_nativeD6Joint = native->is<physx::PxD6Joint>();
            AZ_Assert(m_nativeD6Joint, "Failed to get PhysX D6 Native joint");
            PHYSX_SCENE_WRITE_LOCK(native->getScene());
            m_nativeD6Joint->setMotion(physx::PxD6Axis::eX, ConvertD6JointAxisToPxD6Motion(m_d6Configuration.m_motionX));

            m_nativeD6Joint->setMotion(physx::PxD6Axis::eY, ConvertD6JointAxisToPxD6Motion(m_d6Configuration.m_motionY));

            m_nativeD6Joint->setMotion(physx::PxD6Axis::eZ, ConvertD6JointAxisToPxD6Motion(m_d6Configuration.m_motionZ));

            m_nativeD6Joint->setMotion(physx::PxD6Axis::eTWIST, ConvertD6JointAxisToPxD6Motion(m_d6Configuration.m_motionTwist));

            m_nativeD6Joint->setMotion(physx::PxD6Axis::eSWING1, ConvertD6JointAxisToPxD6Motion(m_d6Configuration.m_motionSwing1));

            m_nativeD6Joint->setMotion(physx::PxD6Axis::eSWING2, ConvertD6JointAxisToPxD6Motion(m_d6Configuration.m_motionSwing2));

            // Set linear limits per axis
            if (m_d6Configuration.m_motionX == D6JointAxis::Limited)
            {
                physx::PxJointLinearLimitPair limitX(m_d6Configuration.m_motionLimitsX.first, m_d6Configuration.m_motionLimitsX.second);
                m_nativeD6Joint->setLinearLimit(physx::PxD6Axis::eX, limitX);
            }

            if (m_d6Configuration.m_motionY == D6JointAxis::Limited)
            {
                physx::PxJointLinearLimitPair limitY(m_d6Configuration.m_motionLimitsY.first, m_d6Configuration.m_motionLimitsY.second);
                m_nativeD6Joint->setLinearLimit(physx::PxD6Axis::eY, limitY);
            }

            if (m_d6Configuration.m_motionZ == D6JointAxis::Limited)
            {
                physx::PxJointLinearLimitPair limitZ(m_d6Configuration.m_motionLimitsZ.first, m_d6Configuration.m_motionLimitsZ.second);
                m_nativeD6Joint->setLinearLimit(physx::PxD6Axis::eZ, limitZ);
            }

            // Set twist limit
            if (m_d6Configuration.m_motionTwist == D6JointAxis::Limited)
            {
                physx::PxJointAngularLimitPair twistLimit(
                    m_d6Configuration.m_motionLimitsTwist.first, m_d6Configuration.m_motionLimitsTwist.second);
                m_nativeD6Joint->setTwistLimit(twistLimit);
            }

            // Set swing limits
            if (m_d6Configuration.m_motionSwing1 == D6JointAxis::Limited || m_d6Configuration.m_motionSwing2 == D6JointAxis::Limited)
            {
                physx::PxJointLimitCone swingLimit(m_d6Configuration.m_motionLimitsCone.first, m_d6Configuration.m_motionLimitsCone.second);
                m_nativeD6Joint->setSwingLimit(swingLimit);
            }

            if (m_d6Configuration.m_driveX.m_driveType != D6JointDriveType::None)
            {
                m_nativeD6Joint->setDrive(physx::PxD6Drive::eX, PhysX::ConvertD6JointDriveToPxD6Drive(m_d6Configuration.m_driveX));
            }

            if (m_d6Configuration.m_driveY.m_driveType != D6JointDriveType::None)
            {
                m_nativeD6Joint->setDrive(physx::PxD6Drive::eY, PhysX::ConvertD6JointDriveToPxD6Drive(m_d6Configuration.m_driveY));
            }

            if (m_d6Configuration.m_driveZ.m_driveType != D6JointDriveType::None)
            {
                m_nativeD6Joint->setDrive(physx::PxD6Drive::eZ, PhysX::ConvertD6JointDriveToPxD6Drive(m_d6Configuration.m_driveZ));
            }
            if (m_d6Configuration.m_driveTwist.m_driveType != D6JointDriveType::None)
            {
                m_nativeD6Joint->setDrive(physx::PxD6Drive::eTWIST, PhysX::ConvertD6JointDriveToPxD6Drive(m_d6Configuration.m_driveTwist));
            }
            if (m_d6Configuration.m_driveSwing.m_driveType != D6JointDriveType::None)
            {
                m_nativeD6Joint->setDrive(physx::PxD6Drive::eSWING, PhysX::ConvertD6JointDriveToPxD6Drive(m_d6Configuration.m_driveSwing));
            }
            if (m_d6Configuration.m_driveSlerp.m_driveType != D6JointDriveType::None)
            {
                m_nativeD6Joint->setDrive(physx::PxD6Drive::eSLERP, PhysX::ConvertD6JointDriveToPxD6Drive(m_d6Configuration.m_driveSlerp));
            }
        }
        const AZ::EntityComponentIdPair entityComponentIdPair(GetEntityId(), GetId());
        JointRequestBus::Handler::BusConnect(entityComponentIdPair);
    }

    void D6JointComponent::DeinitNativeJoint()
    {
        JointRequestBus::Handler::BusDisconnect();
        JointComponent::DeinitNativeJoint();
        m_nativeD6Joint = nullptr;
    }

    float D6JointComponent::GetPosition() const
    {
        AZ_WarningOnce("PhysX", false, "GetPosition not implemented for D6 joint, use GetTransform instead");
        return 0.0f;
    }

    float D6JointComponent::GetVelocity() const
    {
        AZ_WarningOnce("PhysX", false, "GetVelocity not implemented for D6 joint, use GetVelocityGeneral instead");
        return 0.0f;
    }

    AZStd::pair<AZ::Vector3, AZ::Vector3> D6JointComponent::GetVelocityGeneral() const
    {
        if (!m_nativeD6Joint)
        {
            return AZStd::make_pair(AZ::Vector3::CreateZero(), AZ::Vector3::CreateZero());
        }

        PHYSX_SCENE_READ_LOCK(m_nativeD6Joint->getScene());
        physx::PxVec3 linearVel;
        physx::PxVec3 angularVel;
        m_nativeD6Joint->getDriveVelocity(linearVel, angularVel);
        return AZStd::make_pair(AZ::Vector3(linearVel.x, linearVel.y, linearVel.z), AZ::Vector3(angularVel.x, angularVel.y, angularVel.z));
    }

    AZStd::pair<float, float> D6JointComponent::GetLimits() const
    {
        AZ_Warning("Physx", false, "GetLimits not implemented for D6 joint");
        return AZStd::make_pair(0.0f, 0.0f);
    }

    AZStd::pair<float, float> D6JointComponent::GetLimitsAxis(int id) const
    {
        if (!m_nativeD6Joint || !idToPxD6AxisEnum.contains(id))
        {
            return AZStd::make_pair(0.0f, 0.0f);
        }
        const physx::PxD6Axis::Enum pxAxis = idToPxD6AxisEnum.at(id);
        PHYSX_SCENE_READ_LOCK(m_nativeD6Joint->getScene());
        if (pxAxis == physx::PxD6Axis::eX)
        {
            physx::PxJointLinearLimitPair limitX = m_nativeD6Joint->getLinearLimit(physx::PxD6Axis::eX);
            return AZStd::make_pair(limitX.lower, limitX.upper);
        }
        else if (pxAxis == physx::PxD6Axis::eY)
        {
            physx::PxJointLinearLimitPair limitY = m_nativeD6Joint->getLinearLimit(physx::PxD6Axis::eY);
            return AZStd::make_pair(limitY.lower, limitY.upper);
        }
        else if (pxAxis == physx::PxD6Axis::eZ)
        {
            physx::PxJointLinearLimitPair limitZ = m_nativeD6Joint->getLinearLimit(physx::PxD6Axis::eZ);
            return AZStd::make_pair(limitZ.lower, limitZ.upper);
        }
        else if (pxAxis == physx::PxD6Axis::eTWIST)
        {
            physx::PxJointAngularLimitPair twistLimit = m_nativeD6Joint->getTwistLimit();
            return AZStd::make_pair(twistLimit.lower, twistLimit.upper);
        }
        else if (pxAxis == physx::PxD6Axis::eSWING1 || pxAxis == physx::PxD6Axis::eSWING2)
        {
            physx::PxJointLimitCone swingLimit = m_nativeD6Joint->getSwingLimit();
            return AZStd::make_pair(swingLimit.yAngle, swingLimit.zAngle);
        }
        return AZStd::make_pair(0.0f, 0.0f);
    }

    AZ::Transform D6JointComponent::GetTransform() const
    {
        if (!m_nativeD6Joint)
        {
            return AZ::Transform::CreateIdentity();
        }
        PHYSX_SCENE_READ_LOCK(m_nativeD6Joint->getScene());
        const auto local = m_nativeD6Joint->getRelativeTransform();
        return PxMathConvert(local);
    }

    void D6JointComponent::SetVelocity([[maybe_unused]] float velocity)
    {
        AZ_Warning("Physx", false, "SetVelocity not implemented for D6 joint, use SetVelocityGeneral instead");
    }

    void D6JointComponent::SetVelocityGeneral(const AZ::Vector3& linear, const AZ::Vector3& angular)
    {
        if (!m_nativeD6Joint)
        {
            return;
        }

        PHYSX_SCENE_READ_LOCK(m_nativeD6Joint->getScene());
        m_nativeD6Joint->setDriveVelocity(
            physx::PxVec3(linear.GetX(), linear.GetY(), linear.GetZ()), physx::PxVec3(angular.GetX(), angular.GetY(), angular.GetZ()));
    }

    void D6JointComponent::SetMaximumForce([[maybe_unused]] float force)
    {
        AZ_Warning("Physx", false, "GetLimits not implemented for D6 joint");
    }

    void D6JointComponent::SetMaximumForceAxis(int id, float force)
    {
        if (!m_nativeD6Joint || !idToPxD6DriveEnum.contains(id))
        {
            return;
        }
        PHYSX_SCENE_WRITE_LOCK(m_nativeD6Joint->getScene());
        const physx::PxD6Drive::Enum driveAxis = idToPxD6DriveEnum.at(id);
        physx::PxD6JointDrive drive = m_nativeD6Joint->getDrive(driveAxis);
        drive.forceLimit = force;
        m_nativeD6Joint->setDrive(driveAxis, drive);
    }
} // namespace PhysX
