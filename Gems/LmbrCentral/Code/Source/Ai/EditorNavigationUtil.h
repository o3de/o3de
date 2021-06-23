/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace LmbrCentral
{
    /**
     * Request available AgentTypes to populate ComboBox drop down.
     */
    AZStd::vector<AZStd::string> PopulateAgentTypeList();
}
