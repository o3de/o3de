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
#include <AzFramework/Physics/Material/PhysicsMaterialPropertyValue.h>

namespace Physics
{
    //! Runtime Physics material instance.
    class Material
    {
    public:
        AZ_RTTI(Physics::Material, "{B0C593B9-F58E-47BF-856B-2F202A9E8813}");

        Material() = default;
        Material(const MaterialId& id, const AZ::Data::Asset<MaterialAsset>& materialAsset);
        virtual ~Material() = default;

        virtual MaterialPropertyValue GetProperty(AZStd::string_view propertyName) const = 0;
        virtual void SetProperty(AZStd::string_view propertyName, MaterialPropertyValue value) = 0;

        MaterialId GetId() const;

        AZ::Data::Asset<MaterialAsset> GetMaterialAsset() const;

    protected:
        MaterialId m_id;
        AZ::Data::Asset<MaterialAsset> m_materialAsset;
    };
} // namespace Physics
