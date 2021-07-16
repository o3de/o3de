/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
