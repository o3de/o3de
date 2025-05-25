/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::EditorEntityContextRequests);
AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::EditorEntityContextNotification);
AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::EditorLegacyGameModeNotifications);
