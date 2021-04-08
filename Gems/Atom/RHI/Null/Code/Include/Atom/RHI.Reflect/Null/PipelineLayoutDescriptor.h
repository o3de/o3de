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

#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    class ReflectContext;
    
    namespace Null
    {
        class PipelineLayoutDescriptor final
            : public RHI::PipelineLayoutDescriptor
        {
            using Base = RHI::PipelineLayoutDescriptor;
        public:
            AZ_RTTI(PipelineLayoutDescriptor, "{1A456415-2028-42E5-A771-61A0E41CF39D}", Base);
            AZ_CLASS_ALLOCATOR(PipelineLayoutDescriptor, AZ::SystemAllocator, 0);
            static void Reflect(AZ::ReflectContext* context);

            static RHI::Ptr<PipelineLayoutDescriptor> Create();

        private:

            PipelineLayoutDescriptor() = default;
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::PipelineLayoutDescriptor overrides
            void ResetInternal() override;
            HashValue64 GetHashInternal(HashValue64 seed) const override;
            //////////////////////////////////////////////////////////////////////////

            AZ_SERIALIZE_FRIEND();
        };
    }
}
