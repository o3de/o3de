/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>

namespace AtomToolsFramework
{
    class ViewportInputBehaviorControllerInterface
    {
    public:
        virtual ~ViewportInputBehaviorControllerInterface() = default;

        //! Get entityId of viewport camera
        virtual const AZ::EntityId& GetCameraEntityId() const = 0;

        //! Get entityId of object
        virtual const AZ::EntityId& GetObjectEntityId() const = 0;

        //! Get entityId of environment entity
        virtual const AZ::EntityId& GetEnvironmentEntityId() const = 0;

        //! Get actual position where the camera is facing
        virtual const AZ::Vector3& GetObjectPosition() const = 0;

        //! Set camera object position
        //! @param objectPosition world space position to point camera at
        virtual void SetObjectPosition(const AZ::Vector3& objectPosition) = 0;

        //! Get the minimum radius of the object entity bounding box
        virtual float GetObjectRadiusMin() const = 0;

        //! Get the radius of the object entity bounding box
        virtual float GetObjectRadius() const = 0;

        //! Get distance between camera and its object
        virtual float GetObjectDistance() const = 0;

        //! Reset camera to default position and rotation 
        virtual void Reset() = 0;

        //! Modify camera's field of view
        //! @param value field of view in degrees
        virtual void SetFieldOfView(float value) = 0;

        //! Check if camera is looking directly at an object
        virtual bool IsCameraCentered() const = 0;
    };
} // namespace AtomToolsFramework
