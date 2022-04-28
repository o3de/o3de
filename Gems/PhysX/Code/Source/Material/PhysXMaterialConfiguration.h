/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Color.h>

namespace PhysX
{
    //! Enumeration that determines how two materials properties are combined when
    //! processing collisions.
    enum class CombineMode : AZ::u8
    {
        Average,
        Minimum,
        Maximum,
        Multiply
    };

    //! Properties of a PhysX material.
    struct MaterialConfiguration
    {
        AZ_TYPE_INFO(PhysX::MaterialConfiguration, "{675AF04D-CF51-479C-9D6A-4D7E264D1DBE}");

        static void Reflect(AZ::ReflectContext* context);

        static constexpr float MinDensityLimit = 0.01f; //!< Minimum possible value of density.
        static constexpr float MaxDensityLimit = 100000.0f; //!< Maximum possible value of density.

        float m_dynamicFriction = 0.5f;
        float m_staticFriction = 0.5f;
        float m_restitution = 0.5f;
        float m_density = 1000.0f;

        CombineMode m_restitutionCombine = CombineMode::Average;
        CombineMode m_frictionCombine = CombineMode::Average;

        AZ::Color m_debugColor = AZ::Colors::White;
    };
} // namespace PhysX
