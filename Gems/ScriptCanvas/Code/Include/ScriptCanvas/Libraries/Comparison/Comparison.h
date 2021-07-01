/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// This header is only meant to include the nodes and should not contain
// shared code

#include <Libraries/Libraries.h>

#include "EqualTo.h"
#include "NotEqualTo.h"
#include "Less.h"
#include "Greater.h"
#include "Less.h"
#include "LessEqual.h"
#include "GreaterEqual.h"

namespace ScriptCanvas
{
    namespace Library
    {
        struct Comparison 
            : public LibraryDefinition
        {
            AZ_RTTI(Comparison, "{8125A479-DF01-4CDF-B8BF-F0810F69E3C7}", LibraryDefinition);

            static void Reflect(AZ::ReflectContext*);
            static void InitNodeRegistry(NodeRegistry& nodeRegistry);
            static AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors();
        };
    }
}
