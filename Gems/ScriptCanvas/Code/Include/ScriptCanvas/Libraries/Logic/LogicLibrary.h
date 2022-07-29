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
    struct NodeRegistry;

    namespace LogicLibrary
    {
        void Reflect(AZ::ReflectContext* reflection);
        void InitNodeRegistry(NodeRegistry* nodeRegistry);
        AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors();
    } // namespace LogicLibrary
} // namespace ScriptCanvas
