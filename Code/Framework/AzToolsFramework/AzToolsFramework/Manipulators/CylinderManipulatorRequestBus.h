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
    //! Interface for handling cylinder manipulator requests.
    //! Note that radius requests are handled by RadiusManipulatorRequests.
    class CylinderManipulatorRequests: public AZ::EntityComponentBus
    {
    public:
        //! Get the height of the cylinder shape.
        virtual float GetHeight() const = 0;
        //! Set the height of the cylinder shape.
        virtual void SetHeight(float height) = 0;

    protected:
        ~CylinderManipulatorRequests() = default;
    };

    //! Type to inherit to implement CylinderManipulatorRequests
    using CylinderManipulatorRequestBus = AZ::EBus<CylinderManipulatorRequests>;
} // namespace AzToolsFramework

