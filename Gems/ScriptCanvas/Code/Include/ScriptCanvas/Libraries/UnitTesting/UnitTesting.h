/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
 
#include <AzCore/Serialization/SerializeContext.h>

namespace ScriptCanvas
{
    namespace UnitTesting
    {
        using Report = AZStd::string;

        bool ExpectBooleanVersioner(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);
        bool ExpectComparisonVersioner(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);
    } 

} 
