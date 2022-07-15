/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>

#define REFLECT_GENERIC_FUNCTION_NODE(GenericFunction, Guid) \
    struct GenericFunction \
    { \
        AZ_TYPE_INFO(GenericFunction, Guid); \
        static void Reflect(AZ::ReflectContext* reflection) \
        { \
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection)) \
            { \
                serializeContext->Class<GenericFunction>()->Version(0); \
            } \
        } \
    };

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
