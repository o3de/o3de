/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Pass/EditorStatePassSystem.h>

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace Render
    {
        //! Feature processor for Editor Mode Feedback visual effect system.
        class EditorModeFeatureProcessor
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::EditorModeFeatureProcessor, "{78D40D57-F564-4ECD-B9F5-D8C9784B15D0}", RPI::FeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void ApplyRenderPipelineChange(RPI::RenderPipeline* renderPipeline) override;

        private:
            //! Cache the pass request data for creating an editor mode feedback parent pass.
            AZ::Data::Asset<AZ::RPI::AnyAsset> m_parentPassRequestAsset;

            //!
            AZStd::unique_ptr<EditorStatePassSystem> m_editorStatePassSystem;
        };
    } // namespace Render
} // namespace AZ
