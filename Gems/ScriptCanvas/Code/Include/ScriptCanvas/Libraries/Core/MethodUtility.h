/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/containers/map.h>

namespace ScriptCanvas::Grammar
{
    struct FunctionPrototype;
}

namespace ScriptCanvas
{
    AZStd::unordered_map<size_t, const AZ::BehaviorMethod*> GetTupleGetMethodsFromResult(const AZ::BehaviorMethod& method);

    AZStd::unordered_map<size_t, const AZ::BehaviorMethod*> GetTupleGetMethods(const AZ::TypeId& typeId);

    AZ::Outcome<const AZ::BehaviorMethod*, void> GetTupleGetMethod(const AZ::TypeId& typeID, size_t index);
}
