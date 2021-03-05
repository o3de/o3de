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

#include <Atom/RHI.Reflect/Metal/ReflectSystemComponent.h>
#include <Atom/RHI.Reflect/Metal/ShaderStageFunction.h>
#include <Atom/RHI.Reflect/Metal/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/Metal/BufferPoolDescriptor.h>
#include <Atom/RHI.Reflect/Metal/PlatformLimitsDescriptor.h>

namespace AZ
{
    namespace Metal
    {
        void ReflectSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ReflectSystemComponent, AZ::Component>()
                ->Version(1);
            }
            
            PipelineLayoutDescriptor::Reflect(context);
            BufferPoolDescriptor::Reflect(context);
            ShaderStageFunction::Reflect(context);
            PlatformLimitsDescriptor::Reflect(context);
        }
    }
}
