/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Serialization/SerializeContext.h>

#if defined(CAMERA_EDITOR)
#include "EditorCameraComponent.h"
#else
#include "CameraComponent.h"
#endif

#include "CameraComponentController.h"

namespace Camera
{
    namespace ClassConverters
    {
        bool DeprecateCameraComponentWithoutEditor(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // Capture the old values
            float fov = DefaultFoV;
            classElement.GetChildData(AZ::Crc32("Field of View"), fov);

            float nearDistance = DefaultNearPlaneDistance;
            classElement.GetChildData(AZ::Crc32("Near Clip Plane Distance"), nearDistance);

            float farDistance = DefaultFarClipPlaneDistance;
            classElement.GetChildData(AZ::Crc32("Far Clip Plane Distance"), farDistance);

            bool shouldSpecifyFrustum = false;
            classElement.GetChildData(AZ::Crc32("SpecifyDimensions"), shouldSpecifyFrustum);

            float frustumWidth = DefaultFrustumDimension;
            classElement.GetChildData(AZ::Crc32("FrustumWidth"), frustumWidth);

            float frustumHeight = DefaultFrustumDimension;
            classElement.GetChildData(AZ::Crc32("FrustumHeight"), frustumHeight);

            // convert to the new class
#if defined(CAMERA_EDITOR)
            if (classElement.GetName() == AZ::Crc32("m_template"))
            {
                classElement.Convert<EditorCameraComponent>(context);
            }
            else
#else
            classElement.Convert<CameraComponent>(context);
#endif // CAMERA_EDITOR

            // add the new values
            classElement.AddElementWithData(context, "Field of View", fov);
            classElement.AddElementWithData(context, "Near Clip Plane Distance", nearDistance);
            classElement.AddElementWithData(context, "Far Clip Plane Distance", farDistance);
            classElement.AddElementWithData(context, "SpecifyDimensions", shouldSpecifyFrustum);
            classElement.AddElementWithData(context, "FrustumWidth", frustumWidth);
            classElement.AddElementWithData(context, "FrustumHeight", frustumHeight);

            return true;
        }

        bool UpdateCameraComponentToUseController(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // Create a controller, as they now house the camera config
            CameraComponentController controller;
            CameraComponentConfig config = controller.GetConfiguration();

            // Migrate pre-existing configuration to the controller config
            classElement.GetChildData(AZ::Crc32("Field of View"), config.m_fov);
            classElement.GetChildData(AZ::Crc32("Near Clip Plane Distance"), config.m_nearClipDistance);
            classElement.GetChildData(AZ::Crc32("Far Clip Plane Distance"), config.m_farClipDistance);
            classElement.GetChildData(AZ::Crc32("SpecifyDimensions"), config.m_specifyFrustumDimensions);
            classElement.GetChildData(AZ::Crc32("FrustumWidth"), config.m_frustumWidth);
            classElement.GetChildData(AZ::Crc32("FrustumHeight"), config.m_frustumHeight);

            controller.SetConfiguration(config);

            // Remove old fields
            classElement.RemoveElementByName(AZ::Crc32("Field of View"));
            classElement.RemoveElementByName(AZ::Crc32("Near Clip Plane Distance"));
            classElement.RemoveElementByName(AZ::Crc32("Far Clip Plane Distance"));
            classElement.RemoveElementByName(AZ::Crc32("SpecifyDimensions"));
            classElement.RemoveElementByName(AZ::Crc32("FrustumWidth"));
            classElement.RemoveElementByName(AZ::Crc32("FrustumHeight"));

            // Add the controller element
            classElement.AddElementWithData(context, "Controller", controller);

            return true;
        }
    }
} // Camera
