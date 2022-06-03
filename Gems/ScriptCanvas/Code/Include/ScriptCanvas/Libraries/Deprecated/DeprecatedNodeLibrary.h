/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class ReflectContext;
    class ComponentDescriptor;
} // namespace AZ

namespace ScriptCanvas
{
    //! Library holds all deprecated generic function nodes which will be cleaned up all together
    struct DeprecatedNodeLibrary
    {
        static void Reflect(AZ::ReflectContext*);
        static AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors();
    };
} // namespace ScriptCanvas
