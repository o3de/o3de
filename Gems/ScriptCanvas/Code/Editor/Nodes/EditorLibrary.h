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
}

namespace ScriptCanvasEditor
{
    namespace Library
    {
        struct Editor : public ScriptCanvas::Library::LibraryDefinition
        {
            AZ_RTTI(Editor, "{59697735-4B64-4DC5-8380-02B2999FFCFE}", ScriptCanvas::Library::LibraryDefinition);

            static void Reflect(AZ::ReflectContext*);
            static void InitNodeRegistry(ScriptCanvas::NodeRegistry& nodeRegistry);
            static AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors();
        };
    }

    AZStd::vector<AZ::ComponentDescriptor*> GetLibraryDescriptors();
}
