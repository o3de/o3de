/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

#include <PostProcess/PostProcessBase.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        PostProcessBase::PostProcessBase(PostProcessFeatureProcessor* featureProcessor)
            : m_featureProcessor(featureProcessor)
        { }

        RPI::Scene* PostProcessBase::GetParentScene() const
        {
            if (m_featureProcessor)
            {
                return m_featureProcessor->GetParentScene();
            }
            return nullptr;
        }

        RPI::ShaderResourceGroup* PostProcessBase::GetSceneSrg() const
        {
            if (GetParentScene())
            {
                return GetParentScene()->GetShaderResourceGroup().get();
            }
            return nullptr;
        }

        AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> PostProcessBase::GetDefaultViewSrg() const
        {
            if (GetParentScene() &&
                GetParentScene()->GetDefaultRenderPipeline() &&
                GetParentScene()->GetDefaultRenderPipeline()->GetDefaultView())
            {
                return GetParentScene()->GetDefaultRenderPipeline()->GetDefaultView()->GetShaderResourceGroup();
            }
            return nullptr;
        }

    }
}
