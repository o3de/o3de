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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector3.h>

namespace MaterialEditor
{
    class MaterialEditorViewportInputControllerRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Get entityId of viewport camera
        virtual const AZ::EntityId& GetCameraEntityId() const = 0;

        //! Get entityId of camera target
        virtual const AZ::EntityId& GetTargetEntityId() const = 0;

        //! Get entityId of scene's IBL entity
        virtual const AZ::EntityId& GetIblEntityId() const = 0;

        //! Get actual position where the camera is facing
        virtual const AZ::Vector3& GetTargetPosition() const = 0;

        //! Set camera target position
        //! @param targetPosition world space position to point camera at
        virtual void SetTargetPosition(const AZ::Vector3& targetPosition) = 0;

        //! Get distance between camera and its target
        virtual float GetDistanceToTarget() const = 0;

        //! Get minimum and maximum camera distance to the model based on mesh size
        //! @param distanceMin closest camera can be to the target
        //! @param distanceMax furthest camera can be from the target
        virtual void GetExtents(float& distanceMin, float& distanceMax) const = 0;

        //! Reset camera to default position and rotation 
        virtual void Reset() = 0;

        //! Modify camera's field of view
        //! @param value field of view in degrees
        virtual void SetFieldOfView(float value) = 0;

        //! Check if camera is looking directly at a model
        virtual bool IsCameraCentered() const = 0;
    };

    using MaterialEditorViewportInputControllerRequestBus = AZ::EBus<MaterialEditorViewportInputControllerRequests>;
} // namespace MaterialEditor
