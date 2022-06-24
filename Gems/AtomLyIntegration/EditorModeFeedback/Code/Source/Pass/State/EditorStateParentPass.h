/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ParentPass.h>



#include <Pass/State/EditorStateParentPass.h>
#include <Pass/State/EditorStateParentPassData.h>

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>

namespace AZ
{
    namespace Render
    {
        class EditorStateParentPass : public AZ::RPI::ParentPass
        {
        public:
            AZ_RTTI(EditorStateParentPass, "{C66D2D82-B1A2-4CDF-8E4A-C5C733F34E32}", AZ::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(EditorStateParentPass, SystemAllocator, 0);

            virtual ~EditorStateParentPass() = default;

            //! Creates a EditorModeFeedbackParentPass
            static RPI::Ptr<EditorStateParentPass> Create(const RPI::PassDescriptor& descriptor)
            {
                RPI::Ptr<EditorStateParentPass> pass = aznew EditorStateParentPass(descriptor);
                return AZStd::move(pass);
            }

        protected:
            EditorStateParentPass(const RPI::PassDescriptor& descriptor)
                : AZ::RPI::ParentPass(descriptor)
                , m_passDescriptor(descriptor)
            {
            }

            //! Pass behavior overrides
            bool IsEnabled() const override
            {
                //return false;
                const RPI::EditorStateParentPassData* passData =
                    RPI::PassUtils::GetPassData<RPI::EditorStateParentPassData>(m_passDescriptor);
                if (passData == nullptr)
                {
                     AZ_Error(
                         "PassSystem", false, "[EditorStateParentPassData '%s']: Trying to construct without valid EditorStateParentPassData!", GetPathName().GetCStr());
                     return false;
                }
                return passData->editorStatePass->IsEnabled();
            }

        private:
            RPI::PassDescriptor m_passDescriptor;
        };

        class EditorStatePassthroughPass
            : public AZ::RPI::FullscreenTrianglePass
        {
        public:
            AZ_RTTI(EditorStatePassthroughPass, "{03EE6F22-7A28-4D01-9D22-0CC04A66B54D}", AZ::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(EditorStatePassthroughPass, SystemAllocator, 0);

            virtual ~EditorStatePassthroughPass() = default;

            //! Creates a EditorModeFeedbackParentPass
            static RPI::Ptr<EditorStatePassthroughPass> Create(const RPI::PassDescriptor& descriptor)
            {
                RPI::Ptr<EditorStatePassthroughPass> pass = aznew EditorStatePassthroughPass(descriptor);
                return AZStd::move(pass);
            }

        protected:
            EditorStatePassthroughPass(const RPI::PassDescriptor& descriptor)
                : AZ::RPI::FullscreenTrianglePass(descriptor)
                , m_passDescriptor(descriptor)
            {
            }

            //! Pass behavior overrides
            bool IsEnabled() const override
            {
                //return false;
                const RPI::EditorStatePassthroughPassData* passData =
                RPI::PassUtils::GetPassData<RPI::EditorStatePassthroughPassData>(m_passDescriptor);
                if (passData == nullptr)
                {
                    AZ_Error(
                        "PassSystem", false, "[EditorStatePassthroughPassData '%s']: Trying to construct without valid EditorStatePassthroughPassData!", GetPathName().GetCStr());
                    return false;
                }
                return passData->editorStatePass->IsEnabled();
            }

        private:
            RPI::PassDescriptor m_passDescriptor;
        };
    } // namespace Render
} // namespace AZ
