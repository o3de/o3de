/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Null/PipelineLayoutDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ
{
    namespace Null
    {
        void PipelineLayoutDescriptor::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PipelineLayoutDescriptor, Base>()
                    ->Version(4);
            }
        }

        RHI::Ptr<PipelineLayoutDescriptor> PipelineLayoutDescriptor::Create()
        {
            return aznew PipelineLayoutDescriptor();
        }

        void PipelineLayoutDescriptor::ResetInternal()
        {
        }
        
        HashValue64 PipelineLayoutDescriptor::GetHashInternal(HashValue64 seed) const
        {
            return seed;
        }
    }
}
