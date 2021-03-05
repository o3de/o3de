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