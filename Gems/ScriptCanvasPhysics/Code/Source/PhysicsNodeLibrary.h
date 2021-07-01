/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once


#include <ScriptCanvas/Libraries/Libraries.h>

namespace AZ
{
    class ReflectContext;
    class ComponentDescriptor;
} // namespace AZ

namespace ScriptCanvasPhysics
{
    //! Defines the Library for custom Script Canvas nodes needed for physics features. 
    struct PhysicsNodeLibrary : public ScriptCanvas::Library::LibraryDefinition
    {
        AZ_RTTI(PhysicsNodeLibrary, "{FB17C991-5150-4E1D-8ECE-DE5C3E08ACB5}", ScriptCanvas::Library::LibraryDefinition);

        static void Reflect(AZ::ReflectContext*);
        static void InitNodeRegistry(ScriptCanvas::NodeRegistry& nodeRegistry);
        static AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors();
    };
} // namespace ScriptCanvasPhysics
