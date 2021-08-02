/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/DX12/ReflectSystemComponent.h>
#include <Atom/RHI.Reflect/DX12/ShaderStageFunction.h>
#include <Atom/RHI.Reflect/DX12/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/DX12/BufferPoolDescriptor.h>
#include <Atom/RHI.Reflect/DX12/PlatformLimitsDescriptor.h>

namespace AZ
{
    namespace DX12
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
