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
    //! Interface for handling box manipulator requests.
    //! Used by \ref BoxComponentMode.
    class BoxManipulatorRequests : public AZ::EntityComponentBus
    {
    public:
        //! Get the X/Y/Z dimensions of the box shape/collider.
        virtual AZ::Vector3 GetDimensions() const = 0;
        //! Set the X/Y/Z dimensions of the box shape/collider.
        virtual void SetDimensions(const AZ::Vector3& dimensions) = 0;
        //! Get the translation offset of the box relative to the entity position.
        virtual AZ::Vector3 GetTranslationOffset() const = 0;
        //! Set the translation offset of the box relative to the entity position.
        virtual void SetTranslationOffset(const AZ::Vector3& translationOffset) = 0;
        //! Get the transform of the box relative to the entity.
        virtual AZ::Transform GetCurrentLocalTransform() const = 0;
        //! Get the space in which the manipulators are defined.
        virtual AZ::Transform GetManipulatorSpace() const = 0;

    protected:
        ~BoxManipulatorRequests() = default;
    };

    //! Type to inherit to implement BoxManipulatorRequests
    using BoxManipulatorRequestBus = AZ::EBus<BoxManipulatorRequests>;
} // namespace AzToolsFramework
