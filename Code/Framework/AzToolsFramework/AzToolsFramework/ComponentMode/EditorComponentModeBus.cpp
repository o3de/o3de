/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>

AZTF_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests);
AZTF_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS_WITH_TRAITS(AzToolsFramework::ComponentModeFramework::ComponentModeDelegateRequests, AzToolsFramework::ComponentModeFramework::ComponentModeMouseViewportRequests)
 
