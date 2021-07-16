/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/EntityId.h>

namespace Camera
{
    //////////////////////////////////////////////////////////////////////////
    /// Classes that inherit from this one will share the life-cycle API's
    /// with components.  Components that contain the subclasses are expected
    /// to call these methods in their Init/Activate/Deactivate methods
    //////////////////////////////////////////////////////////////////////////
    class ICameraSubComponent
    {
    public:
        AZ_RTTI(ICameraSubComponent, "{5AB43B64-941E-493F-870F-DE313E4D11A0}");
        virtual ~ICameraSubComponent() = default;

        virtual void Init() {}
        virtual void Activate(AZ::EntityId) = 0;
        virtual void Deactivate() = 0;
    };
} //namespace Camera
