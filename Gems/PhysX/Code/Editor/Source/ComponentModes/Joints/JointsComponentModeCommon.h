/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/string/string.h>
#include <Editor/EditorJointCommon.h>

namespace PhysX::JointsComponentModeCommon
{
    namespace SubComponentModes
    {
        //! Used to identify a specific sub component mode used with joints
        enum class ModeType : AZ::u32
        {
            Translation = 0,
            Rotation,
            MaxForce,
            MaxTorque,
            Damping,
            Stiffness,
            TwistLimits,
            SwingLimits,
            SnapPosition,
            SnapRotation,

            ModeCount
        };

        //! Shared data structure used with Angle Cone and Angle Pair sub component modes.
        //! Holds information about the axis and limits of the angle.
        struct AngleModesSharedRotationState
        {
            AZ::Vector3 m_axis; //!< The Axis of rotation to apply the limits.
            AZ::Quaternion m_savedOrientation = AZ::Quaternion::CreateIdentity(); //!< The angle delta of the last mouse action.
            PhysX::AngleLimitsFloatPair m_valuePair; //!< the current limits of the angle.
        };
    } // namespace SubComponentModes

    //! Name Identifiers for the joint components sub modes.
    struct ParamaterNames
    {
        static const AZStd::string_view TwistLimits;
        static const AZStd::string_view Damping;
        static const AZStd::string_view MaxForce;
        static const AZStd::string_view MaxTorque;
        static const AZStd::string_view Position;
        static const AZStd::string_view Rotation;
        static const AZStd::string_view SnapPosition;
        static const AZStd::string_view SnapRotation;
        static const AZStd::string_view Stiffness;
        static const AZStd::string_view SwingLimit;
        static const AZStd::string_view Transform;
        static const AZStd::string_view ComponentMode;
        static const AZStd::string_view LeadEntity;
    };

    //! A pairing of Sub component Names, and Id.
    struct SubModeParamaterState
    {
        SubComponentModes::ModeType m_modeType = SubComponentModes::ModeType::ModeCount; //!< The Id of the sub component mode.
        AZStd::string m_parameterName; //!< The name of the sub component mode.
    };
} // namespace PhysX::JointsComponentModeCommon
