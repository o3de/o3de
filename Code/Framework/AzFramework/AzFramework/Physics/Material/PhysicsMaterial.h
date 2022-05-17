/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Physics/Material/PhysicsMaterialId.h>
#include <AzFramework/Physics/Material/PhysicsMaterialAsset.h>

namespace Physics
{
    //! Runtime Physics material instance.
    // TODO: Material2 is temporary until old Material class is removed.
    class Material2
    {
    public:
        AZ_RTTI(Physics::MaterialAsset2, "{B0C593B9-F58E-47BF-856B-2F202A9E8813}");

        Material2() = default;
        Material2(const MaterialId2& id, const AZ::Data::Asset<MaterialAsset>& materialAsset);
        virtual ~Material2() = default;

        virtual float GetDynamicFriction() const = 0;
        virtual void SetDynamicFriction(float dynamicFriction) = 0;

        virtual float GetStaticFriction() const = 0;
        virtual void SetStaticFriction(float staticFriction) = 0;

        virtual float GetRestitution() const = 0;
        virtual void SetRestitution(float restitution) = 0;

        virtual CombineMode GetFrictionCombineMode() const = 0;
        virtual void SetFrictionCombineMode(CombineMode mode) = 0;

        virtual CombineMode GetRestitutionCombineMode() const = 0;
        virtual void SetRestitutionCombineMode(CombineMode mode) = 0;

        virtual float GetDensity() const = 0;
        virtual void SetDensity(float density) = 0;

        virtual const AZ::Color& GetDebugColor() const = 0;
        virtual void SetDebugColor(const AZ::Color& debugColor) = 0;

        MaterialId2 GetId() const;

        AZ::Data::Asset<MaterialAsset> GetMaterialAsset() const;

    protected:
        MaterialId2 m_id;
        AZ::Data::Asset<MaterialAsset> m_materialAsset;
    };
} // namespace Physics
