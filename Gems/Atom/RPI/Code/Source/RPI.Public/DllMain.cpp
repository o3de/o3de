/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(AZ_MONOLITHIC_BUILD)

#include <AzCore/PlatformDef.h>
#include <AzCore/Serialization/SerializeContext.h>

#endif

#include <Atom/RPI.Public/SceneBus.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>

AZ_INSTANTIATE_EBUS_MULTI_ADDRESS(ATOM_RPI_PUBLIC_API, AZ::RPI::SceneNotification);
AZ_INSTANTIATE_EBUS_MULTI_ADDRESS(ATOM_RPI_PUBLIC_API, AZ::RPI::ShaderReloadNotifications);
