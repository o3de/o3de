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

#include <PhysX/Material/PhysXMaterial.h>

namespace PhysX
{
    //! Properties of compliant contact mode.
    struct CompliantContactModeConfiguration
    {
        AZ_TYPE_INFO(PhysX::CompliantContactModeConfiguration, "{1F38A087-E918-4ED1-AEC5-5FEC25A47AD1}");

        static void Reflect(AZ::ReflectContext* context);

        bool m_enabled = false;
        float m_damping = 1.0f;
        float m_stiffness = 1e+5f;

    private:
        bool ReadOnlyProperties() const;
    };

    //! Properties of a PhysX material.
    struct MaterialConfiguration
    {
        AZ_TYPE_INFO(PhysX::MaterialConfiguration, "{66213D20-9862-465D-AF4F-2D94317161F6}");

        static void Reflect(AZ::ReflectContext* context);

        float m_dynamicFriction = 0.5f;
        float m_staticFriction = 0.5f;
        float m_restitution = 0.5f;
        float m_density = 1000.0f;

        CombineMode m_restitutionCombine = CombineMode::Average;
        CombineMode m_frictionCombine = CombineMode::Average;

        CompliantContactModeConfiguration m_compliantContactMode;

        AZ::Color m_debugColor = AZ::Colors::White;

        //! Creates a Physics Material Asset with random Id from the
        //! properties of material configuration.
        AZ::Data::Asset<Physics::MaterialAsset> CreateMaterialAsset() const;

        static void ValidateMaterialAsset(AZ::Data::Asset<Physics::MaterialAsset> materialAsset);

    private:
        static float GetMinDensityLimit();
        static float GetMaxDensityLimit();

        bool IsRestitutionReadOnly() const;
        AZ::Crc32 GetCompliantConstantModeVisibility() const;
    };
} // namespace PhysX
