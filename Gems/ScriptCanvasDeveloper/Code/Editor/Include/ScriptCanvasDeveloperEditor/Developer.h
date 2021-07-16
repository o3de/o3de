/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Libraries/Libraries.h>

namespace ScriptCanvasDeveloper
{
    namespace Libraries
    {
        struct Developer
            : public ScriptCanvas::Library::LibraryDefinition
        {
            AZ_RTTI(Developer, "{2284A0BC-44A0-47A2-B67C-DF8862CF859B}", ScriptCanvas::Library::LibraryDefinition);

            static void Reflect(AZ::ReflectContext*);
            static void InitNodeRegistry(ScriptCanvas::NodeRegistry& nodeRegistry);
            static AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors();
        };
    }
}
