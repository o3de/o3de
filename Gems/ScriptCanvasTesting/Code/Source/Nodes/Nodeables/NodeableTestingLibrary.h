/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ScriptCanvas/Libraries/Libraries.h>

namespace ScriptCanvasTesting
{
    struct NodeableTestingLibrary : public ScriptCanvas::Library::LibraryDefinition
    {
        AZ_RTTI(NodeableTestingLibrary, "{F48EF27C-EA32-455C-88AB-191A132F093B}", ScriptCanvas::Library::LibraryDefinition);

        static void Reflect(AZ::ReflectContext*);
        static void InitNodeRegistry(ScriptCanvas::NodeRegistry& nodeRegistry);
        static AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors();

        ~NodeableTestingLibrary() override = default;
    };
}
