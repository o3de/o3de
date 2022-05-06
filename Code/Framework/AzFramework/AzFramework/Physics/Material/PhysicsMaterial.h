/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Physics/Material/PhysicsMaterialConfiguration.h>

namespace Physics
{
    //! Runtime Physics material interface.
    // TODO: Material2 is temporary until old Material class is removed.
    class Material2
    {
    public:
        Material2() = default;
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

        virtual const void* GetNativePointer() const = 0;
    };
} // namespace Physics
