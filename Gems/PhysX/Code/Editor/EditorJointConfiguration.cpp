/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <Editor/EditorJointConfiguration.h>
#include <Source/EditorRigidBodyComponent.h>
#include <Source/EditorStaticRigidBodyComponent.h>

namespace
{
    const float LocalRotationMax = 360.0f;
    const float LocalRotationMin = -360.0f;
}

namespace PhysX
{
    const float EditorJointLimitBase::s_springMax = 1000000.0f;
    const float EditorJointLimitBase::s_springMin = 0.001f;
    const float EditorJointLimitBase::s_toleranceMax = 90.0f;
    const float EditorJointLimitBase::s_toleranceMin = 0.001f;

    const float EditorJointLimitPairConfig::s_angleMax = 360.0f;
    const float EditorJointLimitPairConfig::s_angleMin = 0.0f;

    const float EditorJointLimitConeConfig::s_angleMax = 180.0f;
    const float EditorJointLimitConeConfig::s_angleMin = 0.1f;

    const float EditorJointLimitLinearPairConfig::LinearLimitMax = 1000.0f;
    const float EditorJointLimitLinearPairConfig::LinearLimitMin = -1000.0f;

    const float EditorJointConfig::BreakageMax = 10000000.0f;
    const float EditorJointConfig::BreakageMin = 0.01f;

    void EditorJointLimitConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorJointLimitConfig>()
                ->Version(1)
                ->Field("Name", &EditorJointLimitConfig::m_name)
                ->Field("Is Limited", &EditorJointLimitConfig::m_isLimited)
                ->Field("Is Soft Limit", &EditorJointLimitConfig::m_isSoftLimit)
                ->Field("Tolerance", &EditorJointLimitConfig::m_tolerance)
                ->Field("Damping", &EditorJointLimitConfig::m_damping)
                ->Field("Stiffness", &EditorJointLimitConfig::m_stiffness)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PhysX::EditorJointLimitConfig>(
                    "Editor Joint Limit Config Base", "Base joint limit parameters.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(0, &PhysX::EditorJointLimitConfig::m_isLimited, "Limit",
                        "When active, the joint's degrees of freedom are limited.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &EditorJointLimitConfig::IsInComponentMode)
                    ->DataElement(0, &PhysX::EditorJointLimitConfig::m_isSoftLimit, "Soft limit",
                        "When active, motion beyond the joint limit with a spring-like return is allowed.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorJointLimitConfig::m_isLimited)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &EditorJointLimitConfig::IsInComponentMode)
                    ->DataElement(0, &PhysX::EditorJointLimitConfig::m_damping, "Damping",
                        "Dissipation of energy and reduction in spring oscillations when outside the joint limit.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorJointLimitConfig::IsSoftLimited)
                    ->Attribute(AZ::Edit::Attributes::Max, s_springMax)
                    ->Attribute(AZ::Edit::Attributes::Min, s_springMin)
                    ->DataElement(0, &PhysX::EditorJointLimitConfig::m_stiffness, "Stiffness",
                        "The spring's drive relative to the position of the follower when outside the joint limit.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorJointLimitConfig::IsSoftLimited)
                    ->Attribute(AZ::Edit::Attributes::Max, s_springMax)
                    ->Attribute(AZ::Edit::Attributes::Min, s_springMin)
                    ;
            }
        }
    }

    bool EditorJointLimitConfig::IsInComponentMode() const
    {
        return m_inComponentMode;
    }

    bool EditorJointLimitConfig::IsSoftLimited() const
    {
        return m_isSoftLimit && m_isLimited;
    }

    void EditorJointLimitPairConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorJointLimitPairConfig>()
                ->Version(1)
                ->Field("Standard Limit Configuration", &EditorJointLimitPairConfig::m_standardLimitConfig)
                ->Field("Positive Limit", &EditorJointLimitPairConfig::m_limitPositive)
                ->Field("Negative Limit", &EditorJointLimitPairConfig::m_limitNegative)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PhysX::EditorJointLimitPairConfig>(
                    "Angular Limit", "Rotation limitation.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &PhysX::EditorJointLimitPairConfig::m_standardLimitConfig
                        , "Standard limit configuration"
                        , "Common limit parameters to all joint types.")
                    ->DataElement(0, &PhysX::EditorJointLimitPairConfig::m_limitPositive, "Positive angular limit",
                        "Positive rotation angle.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorJointLimitPairConfig::IsLimited)
                    ->Attribute(AZ::Edit::Attributes::Max, s_angleMax)
                    ->Attribute(AZ::Edit::Attributes::Min, s_angleMin)
                    ->DataElement(0, &PhysX::EditorJointLimitPairConfig::m_limitNegative, "Negative angular limit",
                        "Negative rotation angle.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorJointLimitPairConfig::IsLimited)
                    ->Attribute(AZ::Edit::Attributes::Max, s_angleMin)
                    ->Attribute(AZ::Edit::Attributes::Min, -s_angleMax)
                    ;
            }
        }
    }

    bool EditorJointLimitPairConfig::IsLimited() const
    {
        return m_standardLimitConfig.m_isLimited;
    }

    JointLimitProperties EditorJointLimitPairConfig::ToGameTimeConfig() const
    {
        return JointLimitProperties(m_standardLimitConfig.m_isLimited
            , m_standardLimitConfig.m_isSoftLimit
            , m_standardLimitConfig.m_damping
            , m_limitPositive, m_limitNegative
            , m_standardLimitConfig.m_stiffness
            , m_standardLimitConfig.m_tolerance);
    }

    void EditorJointLimitLinearPairConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorJointLimitLinearPairConfig>()
                ->Version(1)
                ->Field("Standard Limit Configuration", &EditorJointLimitLinearPairConfig::m_standardLimitConfig)
                ->Field("Lower Limit", &EditorJointLimitLinearPairConfig::m_limitLower)
                ->Field("Upper Limit", &EditorJointLimitLinearPairConfig::m_limitUpper)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PhysX::EditorJointLimitLinearPairConfig>("Linear Limit", "Limitation on linear motion.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        0,
                        &PhysX::EditorJointLimitLinearPairConfig::m_standardLimitConfig,
                        "Standard limit configuration",
                        "Common limit parameters to all joint types.")
                    ->DataElement(
                        0, &PhysX::EditorJointLimitLinearPairConfig::m_limitLower, "Lower linear limit", "Lower limit of linear motion.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorJointLimitLinearPairConfig::IsLimited)
                    ->Attribute(AZ::Edit::Attributes::Max, LinearLimitMax)
                    ->Attribute(AZ::Edit::Attributes::Min, LinearLimitMin)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorJointLimitLinearPairConfig::OnLimitLowerChanged)
                    ->DataElement(
                        0, &PhysX::EditorJointLimitLinearPairConfig::m_limitUpper, "Upper linear limit", "Upper limit of linear motion.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorJointLimitLinearPairConfig::IsLimited)
                    ->Attribute(AZ::Edit::Attributes::Max, LinearLimitMax)
                    ->Attribute(AZ::Edit::Attributes::Min, LinearLimitMin)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorJointLimitLinearPairConfig::OnLimitUpperChanged);
            }
        }
    }

    AZ::Crc32 EditorJointLimitLinearPairConfig::OnLimitLowerChanged()
    {
        // force lower limit to be <= upper limit
        m_limitLower = AZ::GetMin(m_limitLower, m_limitUpper);
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }

    AZ::Crc32 EditorJointLimitLinearPairConfig::OnLimitUpperChanged()
    {
        // force upper limit to be >= lower limit
        m_limitUpper = AZ::GetMax(m_limitLower, m_limitUpper);
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }

    bool EditorJointLimitLinearPairConfig::IsLimited() const
    {
        return m_standardLimitConfig.m_isLimited;
    }

    JointLimitProperties EditorJointLimitLinearPairConfig::ToGameTimeConfig() const
    {
        return JointLimitProperties(
            m_standardLimitConfig.m_isLimited,
            m_standardLimitConfig.m_isSoftLimit,
            m_standardLimitConfig.m_damping,
            m_limitUpper,
            m_limitLower,
            m_standardLimitConfig.m_stiffness,
            m_standardLimitConfig.m_tolerance);
    }

    void EditorJointLimitConeConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorJointLimitConeConfig>()
                ->Version(1)
                ->Field("Standard Limit Configuration", &EditorJointLimitConeConfig::m_standardLimitConfig)
                ->Field("Y Axis Limit", &EditorJointLimitConeConfig::m_limitY)
                ->Field("Z Axis Limit", &EditorJointLimitConeConfig::m_limitZ)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PhysX::EditorJointLimitConeConfig>(
                    "Angular Limit", "Rotation limitation.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &PhysX::EditorJointLimitConeConfig::m_standardLimitConfig
                        , "Standard limit configuration"
                        , "Common limit parameters to all joint types.")
                    ->DataElement(0, &PhysX::EditorJointLimitConeConfig::m_limitY, "Y axis angular limit",
                        "Limit for swing angle about Y axis.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorJointLimitConeConfig::IsLimited)
                    ->Attribute(AZ::Edit::Attributes::Max, s_angleMax)
                    ->Attribute(AZ::Edit::Attributes::Min, s_angleMin)
                    ->DataElement(0, &PhysX::EditorJointLimitConeConfig::m_limitZ, "Z axis angular limit",
                        "Limit for swing angle about Z axis.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorJointLimitConeConfig::IsLimited)
                    ->Attribute(AZ::Edit::Attributes::Max, s_angleMax)
                    ->Attribute(AZ::Edit::Attributes::Min, s_angleMin)
                    ;
            }
        }
    }

    bool EditorJointLimitConeConfig::IsLimited() const
    {
        return m_standardLimitConfig.m_isLimited;
    }

    JointLimitProperties EditorJointLimitConeConfig::ToGameTimeConfig() const
    {
        return JointLimitProperties(m_standardLimitConfig.m_isLimited
            , m_standardLimitConfig.m_isSoftLimit
            , m_standardLimitConfig.m_damping
            , m_limitY
            , m_limitZ
            , m_standardLimitConfig.m_stiffness
            , m_standardLimitConfig.m_tolerance);
    }

    void EditorJointConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorJointConfig>()
                ->Version(1)
                ->Field("Local Position", &EditorJointConfig::m_localPosition)
                ->Field("Local Rotation", &EditorJointConfig::m_localRotation)
                ->Field("Fix Joint Location", &EditorJointConfig::m_fixJointLocation)
                ->Field("Parent Entity", &EditorJointConfig::m_leadEntity)
                ->Field("Child Entity", &EditorJointConfig::m_followerEntity)
                ->Field("Breakable", &EditorJointConfig::m_breakable)
                ->Field("Maximum Force", &EditorJointConfig::m_forceMax)
                ->Field("Maximum Torque", &EditorJointConfig::m_torqueMax)
                ->Field("Display Debug", &EditorJointConfig::m_displayJointSetup)
                ->Field("Select Lead on Snap", &EditorJointConfig::m_selectLeadOnSnap)
                ->Field("Self Collide", &EditorJointConfig::m_selfCollide)
                ;

            serializeContext->Enum<EditorJointConfig::DisplaySetupState>()
                ->Value("Never", EditorJointConfig::DisplaySetupState::Never)
                ->Value("Selected", EditorJointConfig::DisplaySetupState::Selected)
                ->Value("Always", EditorJointConfig::DisplaySetupState::Always)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Enum<EditorJointConfig::DisplaySetupState>("Joint Display Setup State", "Options for displaying joint setup.")
                    ->Value("Never", EditorJointConfig::DisplaySetupState::Never)
                    ->Value("Selected", EditorJointConfig::DisplaySetupState::Selected)
                    ->Value("Always", EditorJointConfig::DisplaySetupState::Always)
                    ;

                editContext->Class<PhysX::EditorJointConfig>(
                    "PhysX Joint Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &PhysX::EditorJointConfig::m_localPosition, "Local Position"
                        , "Local Position of joint, relative to its entity.")
                    ->DataElement(0, &PhysX::EditorJointConfig::m_localRotation, "Local Rotation"
                        , "Local Rotation of joint, relative to its entity.")
                    ->Attribute(AZ::Edit::Attributes::Min, LocalRotationMin)
                    ->Attribute(AZ::Edit::Attributes::Max, LocalRotationMax)
                    ->DataElement(0, &PhysX::EditorJointConfig::m_fixJointLocation, "Fix Joint Location"
                        , "When enabled the joint will remain in the same location when moving the entity.")
                    ->DataElement(0, &PhysX::EditorJointConfig::m_leadEntity, "Lead Entity"
                        , "Parent entity associated with joint.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorJointConfig::OnLeadEntityChanged)
                    ->DataElement(0, &PhysX::EditorJointConfig::m_selfCollide, "Lead-Follower Collide"
                        , "When active, the lead and follower pair will collide with each other.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &PhysX::EditorJointConfig::m_displayJointSetup, "Display Setup in Viewport"
                        , "Never = Not shown."
                        "Select = Show setup display when entity is selected."
                        "Always = Always show setup display.")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &EditorJointConfig::IsInComponentMode)
                    ->DataElement(0, &PhysX::EditorJointConfig::m_selectLeadOnSnap, "Select Lead on Snap"
                        , "Select lead entity on snap to position in component mode.")
                    ->DataElement(0, &PhysX::EditorJointConfig::m_breakable
                        , "Breakable"
                        , "Joint is breakable when force or torque exceeds limit.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &EditorJointConfig::IsInComponentMode)
                    ->DataElement(0, &PhysX::EditorJointConfig::m_forceMax,
                        "Maximum Force", "Amount of force joint can withstand before breakage.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorJointConfig::m_breakable)
                    ->Attribute(AZ::Edit::Attributes::Max, BreakageMax)
                    ->Attribute(AZ::Edit::Attributes::Min, BreakageMin)
                    ->DataElement(0, &PhysX::EditorJointConfig::m_torqueMax,
                        "Maximum Torque", "Amount of torque joint can withstand before breakage.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorJointConfig::m_breakable)
                    ->Attribute(AZ::Edit::Attributes::Max, BreakageMax)
                    ->Attribute(AZ::Edit::Attributes::Min, BreakageMin)
                    ;
            }
        }
    }

    void EditorJointConfig::SetLeadEntityId(AZ::EntityId leadEntityId)
    {
        m_leadEntity = leadEntityId;
        OnLeadEntityChanged();

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay
            , AzToolsFramework::Refresh_AttributesAndValues);
    }

    JointGenericProperties EditorJointConfig::ToGenericProperties() const
    {
        JointGenericProperties::GenericJointFlag flags = JointGenericProperties::GenericJointFlag::None;
        if (m_breakable)
        {
            flags |= JointGenericProperties::GenericJointFlag::Breakable;
        }
        if (m_selfCollide)
        {
            flags |= JointGenericProperties::GenericJointFlag::SelfCollide;
        }

        return JointGenericProperties(flags, m_forceMax, m_torqueMax);
    }

    JointComponentConfiguration EditorJointConfig::ToGameTimeConfig() const
    {
        return JointComponentConfiguration(
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateFromEulerAnglesDegrees(m_localRotation),
                m_localPosition),
            m_leadEntity,
            m_followerEntity);
    }

    bool EditorJointConfig::ShowSetupDisplay() const
    {
        switch(m_displayJointSetup)
        {
        case DisplaySetupState::Always:
            return true;
        case DisplaySetupState::Selected:
            {
                bool showSetup = false;
                AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                    showSetup, m_followerEntity, &AzToolsFramework::EditorEntityInfoRequests::IsSelected);
                return showSetup;
            }
        }
        return false;
    }

    bool EditorJointConfig::IsInComponentMode() const
    {
        return m_inComponentMode;
    }

    AZ::Crc32 EditorJointConfig::OnLeadEntityChanged() const
    {
        if (!m_leadEntity.IsValid())
        {
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity,
            &AZ::ComponentApplicationRequests::FindEntity,
            m_leadEntity);
        if (entity)
        {
            [[maybe_unused]] const bool leadEntityHasRigidActor =
                (entity->FindComponent<PhysX::EditorRigidBodyComponent>() ||
                 entity->FindComponent<PhysX::EditorStaticRigidBodyComponent>());

            AZ_Warning("EditorJointComponent",
                leadEntityHasRigidActor,
                "Joints require either a dynamic or static rigid body on the lead entity. "
                "Please add either a static or a dynamic rigid body component to entity %s",
                entity->GetName().c_str());
        }
        else
        {
            AZStd::string followerEntityName;
            if (m_followerEntity.IsValid())
            {
                AZ::ComponentApplicationBus::BroadcastResult(followerEntityName,
                    &AZ::ComponentApplicationRequests::GetEntityName,
                    m_followerEntity);
            }

            AZ_Warning("EditorJointComponent",
                false,
                "Cannot find instance of lead entity given its entity ID. Please check that joint in entity %s has valid lead entity.",
                followerEntityName.c_str());
        }

        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }
} // namespace PhysX
