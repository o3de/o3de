/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace AzToolsFramework
{
    //! Interface for handling radius manipulator requests.
    class RadiusManipulatorRequests : public AZ::EntityComponentBus
    {
    public:
        //! Get the radius of the shape.
        virtual float GetRadius() const = 0;
        //! Set the radius of the shape.
        virtual void SetRadius(float radius) = 0;

    protected:
        ~RadiusManipulatorRequests() = default;
    };

    //! Type to inherit to implement RadiusManipulatorRequests
    using RadiusManipulatorRequestBus = AZ::EBus<RadiusManipulatorRequests>;
} // namespace AzToolsFramework
