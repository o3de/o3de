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