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
        virtual AZ::Vector3 GetDimensions() = 0;
        //! Set the X/Y/Z dimensions of the box shape/collider.
        virtual void SetDimensions(const AZ::Vector3& dimensions) = 0;
        // O3DE_DEPRECATION_NOTICE(GHI-7572)
        //! @deprecated Because non-uniform scale effects can be complex, it is recommended to separately use
        //! AZ::TransformBus::Events::GetWorldTM, AZ::NonUniformScaleRequests::GetScale and GetCurrentLocalTransform
        //! and combine their effects.
        //! Get the transform of the box shape/collider.
        //! This is used by \ref BoxComponentMode instead of the \ref \AZ::TransformBus
        //! because a collider may have an additional translation/orientation offset from
        //! the Entity transform.
        virtual AZ::Transform GetCurrentTransform() = 0;
        //! Get the transform of the box relative to the entity.
        virtual AZ::Transform GetCurrentLocalTransform() = 0;
        // O3DE_DEPRECATION_NOTICE(GHI-7572)
        //! @deprecated Because non-uniform scale effects can be complex, it is recommended to separately use
        //! AZ::TransformBus::Events::GetWorldTM, AZ::NonUniformScaleRequests::GetScale and GetCurrentLocalTransform
        //! and combine their effects.
        //! Get the scale currently applied to the box.
        //! With the Box Shape, the largest x/y/z component is taken
        //! so scale is always uniform, with colliders the scale may
        //! be different per component.
        virtual AZ::Vector3 GetBoxScale() = 0;

    protected:
        ~BoxManipulatorRequests() = default;
    };

    //! Type to inherit to implement BoxManipulatorRequests
    using BoxManipulatorRequestBus = AZ::EBus<BoxManipulatorRequests>;
} // namespace AzToolsFramework
