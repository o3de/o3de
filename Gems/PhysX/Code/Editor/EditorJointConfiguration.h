/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Source/JointComponent.h>

namespace PhysX
{
    /// Base class for joint limits.
    class EditorJointLimitBase
    {
    public:
        AZ_CLASS_ALLOCATOR(EditorJointLimitBase, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(EditorJointLimitBase, "{7D6BD28B-6DAF-42F7-8EFF-0F5ACBBDBAE7}");

        static const float s_springMax; ///< Maximum value for spring stiffness and damping.
        static const float s_springMin; ///< Minimum value for spring stiffness and damping.
        static const float s_toleranceMax; ///< Maximum value for limit tolerance, distance at which limit gets activated/enforced.
        static const float s_toleranceMin; ///< Minimum value for limit tolerance, distance at which limit gets activated/enforced.
    };

    class EditorJointLimitConfig : public EditorJointLimitBase
    {
    public:
        AZ_CLASS_ALLOCATOR(EditorJointLimitConfig, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(EditorJointLimitConfig, "{3A874895-D9A7-404A-95E4-8C05D032FA0B}");
        static void Reflect(AZ::ReflectContext* context);

        bool IsSoftLimited() const;

        AZStd::string m_name;
        bool m_inComponentMode = false;
        bool m_isLimited = true; ///< Indicates if this joint has limits, e.g. maximum swing angles.
        bool m_isSoftLimit = false; ///< If limit is soft, spring and damping are taken into account.
        float m_tolerance = 0.1f; ///< Field is not shown in the editor. May not be easy for users to utilize this value.
        float m_damping = 20.0f;
        float m_stiffness = 100.0f;

    private:
        static bool VersionConverter(AZ::SerializeContext& context,
            AZ::SerializeContext::DataElementNode& classElement);

        bool IsInComponentMode() const; ///< This function is necessary for usage of m_inComponentMode as an attribute in the edit context. Using the variable directly instead of this function will result in the variable being saved.
    };

    /// Pair (angles) limits for joints.
    class EditorJointLimitPairConfig : public EditorJointLimitBase
    {
    public:
        static const float s_angleMax;
        static const float s_angleMin;

        AZ_CLASS_ALLOCATOR(EditorJointLimitPairConfig, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(EditorJointLimitPairConfig, "{319BD38C-A48F-43E2-B7F5-E6E40C88C61C}");
        static void Reflect(AZ::ReflectContext* context);

        bool IsLimited() const;
        JointLimitProperties ToGameTimeConfig() const;

        EditorJointLimitConfig m_standardLimitConfig;
        float m_limitPositive = 45.0f;
        float m_limitNegative = -45.0f;
    };

    /// Cone (swing) limits for joints.
    class EditorJointLimitConeConfig : public EditorJointLimitBase
    {
    public:
        static const float s_angleMax;
        static const float s_angleMin;

        AZ_CLASS_ALLOCATOR(EditorJointLimitConeConfig, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(EditorJointLimitConeConfig, "{FF481FEF-7033-440B-8046-B459AC309976}");
        static void Reflect(AZ::ReflectContext* context);

        bool IsLimited() const;
        JointLimitProperties ToGameTimeConfig() const;

        EditorJointLimitConfig m_standardLimitConfig;
        float m_limitY = 45.0f;
        float m_limitZ = 45.0f;
    };

    class EditorJointConfig
    {
    public:
        static const float s_breakageMax;
        static const float s_breakageMin;

        AZ_CLASS_ALLOCATOR(EditorJointConfig, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(EditorJointConfig, "{8A966D65-CA97-4786-A13C-ACAA519D97EA}");
        static void Reflect(AZ::ReflectContext* context);

        enum class DisplaySetupState : AZ::u8
        {
            Never = 0,
            Selected,
            Always
        };

        void SetLeadEntityId(AZ::EntityId leadEntityId);
        JointGenericProperties ToGenericProperties() const;
        JointComponentConfiguration ToGameTimeConfig() const;

        bool ShowSetupDisplay() const;

        bool m_breakable = false;
        DisplaySetupState m_displayJointSetup = DisplaySetupState::Selected;
        bool m_inComponentMode = false;
        bool m_selectLeadOnSnap = true;
        bool m_selfCollide = false;

        AZ::EntityId m_leadEntity;
        AZ::EntityId m_followerEntity;

        float m_forceMax = 1.0f;
        float m_torqueMax = 1.0f;

        AZ::Vector3 m_localPosition = AZ::Vector3::CreateZero();
        AZ::Vector3 m_localRotation = AZ::Vector3::CreateZero(); ///< Local rotation angles about X, Y, Z axes in degrees, relative to lead body.

    private:
        static bool VersionConverter(AZ::SerializeContext& context,
            AZ::SerializeContext::DataElementNode& classElement);

        bool IsInComponentMode() const; ///< This function is necessary for usage of m_inComponentMode as an attribute in the edit context. Using the variable directly instead of this function will result in the variable being saved.

        void ValidateLeadEntityId(); ///< Issues warning if lead entity does not contain required components for a joint to function correctly.
    };

} // namespace PhysX

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(PhysX::EditorJointConfig::DisplaySetupState, "{17EBE6BD-289A-4326-8A24-DCE3B7FEC51E}");
} // namespace AZ
