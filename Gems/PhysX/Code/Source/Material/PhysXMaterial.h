/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Material/PhysXMaterialConfiguration.h>

#include <PxPhysicsAPI.h>

namespace PhysX
{
    //! Runtime PhysX material, created from a MaterialConfiguration.
    // TODO: Material2 is temporary until old Material class is removed.
    class Material2
    {
    public:
        Material2(const MaterialConfiguration& configuration);

        float GetDynamicFriction() const;
        void SetDynamicFriction(float dynamicFriction);

        float GetStaticFriction() const;
        void SetStaticFriction(float staticFriction);

        float GetRestitution() const;
        void SetRestitution(float restitution);

        CombineMode GetFrictionCombineMode() const;
        void SetFrictionCombineMode(CombineMode mode);

        CombineMode GetRestitutionCombineMode() const;
        void SetRestitutionCombineMode(CombineMode mode);

        float GetDensity() const;
        void SetDensity(float density);

        const AZ::Color& GetDebugColor() const;
        void SetDebugColor(const AZ::Color& debugColor);

        const void* GetNativePointer() const;

    private:
        using PxMaterialUniquePtr = AZStd::unique_ptr<physx::PxMaterial, AZStd::function<void(physx::PxMaterial*)>>;

        PxMaterialUniquePtr m_pxMaterial;
        float m_density = 1000.0f;
        AZ::Color m_debugColor = AZ::Colors::White;
    };
} // namespace PhysX
