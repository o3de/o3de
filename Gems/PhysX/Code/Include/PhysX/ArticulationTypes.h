/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/base.h>

namespace PhysX
{
    //! Joint type, which determines which degrees of freedom the joint has.
    //! This is just a wrapper for a PhysX SDK enum, to allow using the O3DE integration without including PhysX headers.
    enum class ArticulationJointType : AZ::u8
    {
        Fix, //!< A joint with no degrees of freedom. 
        Hinge, //!< A joint with one rotational degree of freedom around the shared X axis of the two bodies.
        Prismatic, //!< A joint with one linear degree of freedom along the shared X axis of the two bodies.
        Unsupported //!< Not yet supported joint types.
    };

    //! Joint axis, used to describe individual degrees of freedom. 
    //! This is just a wrapper for a PhysX SDK enum, to allow using the O3DE integration without including PhysX headers.
    enum class ArticulationJointAxis : AZ::u8
    {
        Twist, //!< Rotational motion about the X axis.
        SwingY, //!< Rotational motion about the Y axis.
        SwingZ, //!< Rotational motion about the Z axis.
        X, //!< Linear motion parallel to the X axis.
        Y, //!< Linear motion parallel to the Y axis.
        Z, //!< Linear motion parallel to the Z axis.
    };

    //! Joint motion type, indicating whether a degree of freedom is disabled, enabled with limits or enabled without limits.
    //! This is just a wrapper for a PhysX SDK enum, to allow using the O3DE integration without including PhysX headers.
    enum class ArticulationJointMotionType : AZ::u8
    {
        Locked, //!< The joint cannot move.
        Limited, //!< The joint can move within its limits.
        Free //!< The joint can move without any limits.
    };
} // namespace PhysX
