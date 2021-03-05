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