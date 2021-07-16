/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
// script canvas
#include <ScriptCanvas/Libraries/Libraries.h>


namespace AZ
{
    class ReflectContext;
    class ComponentDescriptor;
} // namespace AZ

namespace StartingPointInput
{
    //////////////////////////////////////////////////////////////////////////
    /// This defines the Library for Input.  
    /// Add your custom nodes like this:
    /// ScriptCanvas::Library::AddNodeToRegistry<InputLibrary, InputNode>(nodeRegistry);
    //////////////////////////////////////////////////////////////////////////
    struct InputLibrary : public ScriptCanvas::Library::LibraryDefinition
    {
        AZ_RTTI(InputLibrary, "{0F7E1590-C2D1-4979-9B51-21576667A514}", ScriptCanvas::Library::LibraryDefinition);

        static void Reflect(AZ::ReflectContext*);
        static void InitNodeRegistry(ScriptCanvas::NodeRegistry& nodeRegistry);
        static AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors();
    };
} // namespace Input
