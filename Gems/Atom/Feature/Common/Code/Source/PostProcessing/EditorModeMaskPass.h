/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        /**
         *  
         */
        class EditorModeMaskPass
            : public AZ::RPI::RasterPass
            //TODO: , public PostProcessingShaderOptionBase
        {
        public:
            AZ_RTTI(EditorModeMaskPass, "{890482AB-192E-45B6-866C-76CB7C799CF3}", AZ::RPI::RasterPass);
            AZ_CLASS_ALLOCATOR(EditorModeMaskPass, SystemAllocator, 0);

            virtual ~EditorModeMaskPass() = default;

            //! Creates a EditorModeMaskPass
            static RPI::Ptr<EditorModeMaskPass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            EditorModeMaskPass(const RPI::PassDescriptor& descriptor);
            
            //! Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            bool IsEnabled() const override;

        private:
        };
    }   // namespace Render
}   // namespace AZ
