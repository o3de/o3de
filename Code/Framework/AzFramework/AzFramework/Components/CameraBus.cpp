/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/CameraBus.h>

AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(AZF_API,Camera::CameraComponentRequests);
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZF_API,Camera::CameraRequests);
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZF_API,Camera::CameraSystemRequests);
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZF_API,Camera::ActiveCameraRequests);
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZF_API,Camera::CameraNotifications);
