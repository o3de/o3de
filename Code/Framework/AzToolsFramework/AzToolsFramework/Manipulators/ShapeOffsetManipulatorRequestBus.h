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
    class Vector3;
}

namespace AzToolsFramework
{
    //! Interface for handling shape offset manipulator requests.
    class ShapeOffsetManipulatorRequests : public AZ::EntityComponentBus
    {
    public:
        //! Get the translation offset of the shape relative to the entity position.
        virtual AZ::Vector3 GetTranslationOffset() const = 0;
        //! Set the translation offset of the shape relative to the entity position.
        virtual void SetTranslationOffset(const AZ::Vector3& translationOffset) = 0;

    protected:
        ~ShapeOffsetManipulatorRequests() = default;
    };

    //! Type to inherit to implement ShapeOffsetManipulatorRequests
    using ShapeOffsetManipulatorRequestBus = AZ::EBus<ShapeOffsetManipulatorRequests>;
} // namespace AzToolsFramework
