/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/Memory.h>

namespace AZ
{
    class ReflectContext;
} // namespace AZ

namespace WhiteBox
{
    // How the white box rigid body should be represented in physics.
    enum class WhiteBoxBodyType
    {
        Static,
        Kinematic
    };

    //! Configuration information to use when setting up a WhiteBoxCollider.
    struct WhiteBoxColliderConfiguration
    {
        AZ_CLASS_ALLOCATOR_DECL

        AZ_TYPE_INFO(WhiteBoxColliderConfiguration, "{36DCCE5D-2E26-4FEE-9A17-6B1D401CE46F}")
        static void Reflect(AZ::ReflectContext* context);

        WhiteBoxBodyType m_bodyType = WhiteBoxBodyType::Static; //!< Default the body type to Static.
    };
} // namespace WhiteBox
