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

#include <AzCore/Component/Component.h>

namespace AZ
{
    namespace Render
    {
        // Provides an interface to the Editor Reflection Probe component
        class EditorReflectionProbeInterface
            : public ComponentBus
        {
            public:
                static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
                
                virtual ~EditorReflectionProbeInterface() = default;
                
                // Signals the component to bake the reflection probe
                virtual AZ::u32 BakeReflectionProbe() = 0;
        };
        
        using EditorReflectionProbeBus = AZ::EBus<EditorReflectionProbeInterface>;
    }
}
