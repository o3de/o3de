/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/StringFunc/StringFunc.h>

namespace AzFramework
{
    // Note that StringFunc has been promoted from AzFramework to AzCore
    // This was done to allow AzCore classes access to basic string processing methods
    namespace StringFunc = AZ::StringFunc;
}
