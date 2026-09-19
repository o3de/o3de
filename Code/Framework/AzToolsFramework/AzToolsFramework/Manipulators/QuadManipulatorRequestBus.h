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
    //! Interface for handling shape offset manipulator requests.
    class QuadManipulatorRequests : public AZ::EntityComponentBus
    {
    public:
        //! Get the width of the shape.
        virtual float GetWidth() const = 0;
        //! Set the width of the shape.
        virtual void SetWidth(float width) = 0;
        //! Get the height of the shape.
        virtual float GetHeight() const = 0;
        //! Set the height of the shape.
        virtual void SetHeight(float width) = 0;
    protected:
        ~QuadManipulatorRequests() = default;
    };

    //! Type to inherit to implement ShapeOffsetManipulatorRequests
    using QuadManipulatorRequestBus = AZ::EBus<QuadManipulatorRequests>;
} // namespace AzToolsFramework

