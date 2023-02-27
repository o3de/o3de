/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    class Quaternion;
} // namespace AZ

namespace AzToolsFramework
{
    //! Interface for handling capsule manipulator requests.
    //! Note that radius requests are handled by RadiusManipulatorRequests.
    class CapsuleManipulatorRequests: public AZ::EntityComponentBus
    {
    public:
        //! Get the height of the capsule shape.
        virtual float GetHeight() const = 0;
        //! Set the height of the capsule shape.
        virtual void SetHeight(float height) = 0;

    protected:
        ~CapsuleManipulatorRequests() = default;
    };

    //! Type to inherit to implement CapsuleManipulatorRequests
    using CapsuleManipulatorRequestBus = AZ::EBus<CapsuleManipulatorRequests>;
} // namespace AzToolsFramework
