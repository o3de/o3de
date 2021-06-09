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

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>

namespace AZ
{
    namespace Render
    {
        /**
         * Fullscreen pass as a base for children passes to the DisplayMapperPass.
         * Children fullscreen passes created by the DisplayMapper subclass this and it provides
         * a way to tidy up the code for connecting to the input attachment of this pass with an
         * attachment from another pass. This is provided as the reference pass and reference
         * attachment. These are determined when the children passes are created by the DisplayMapper.
         */
        class DisplayMapperFullScreenPass
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(DisplayMapperFullScreenPass);

        public:
            AZ_RTTI(DisplayMapperFullScreenPass, "{11E9EBE0-DF6D-4260-9281-C99E386231BF}", RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(DisplayMapperFullScreenPass, SystemAllocator, 0);
            virtual ~DisplayMapperFullScreenPass();

            /// Creates a DisplayMapperFullScreenPass
            static RPI::Ptr<DisplayMapperFullScreenPass> Create(const RPI::PassDescriptor& descriptor);

            void SetInputReferencePassName(const Name& passName);
            void SetInputReferenceAttachmentName(const Name& attachmentName);

        protected:
            explicit DisplayMapperFullScreenPass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            virtual void BuildInternal() override;

        private:
            Name m_inputReferencePassName = Name{ "Parent" };
            Name m_inputReferenceAttachmentName = Name{ "Input" };
            const Name InputAttachmentName = Name{ "Input" };
        };
    }   // namespace Render
}   // namespace AZ
