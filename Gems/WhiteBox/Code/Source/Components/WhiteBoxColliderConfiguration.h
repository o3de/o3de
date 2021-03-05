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
