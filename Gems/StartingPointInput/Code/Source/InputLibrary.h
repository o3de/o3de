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
