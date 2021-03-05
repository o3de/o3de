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
