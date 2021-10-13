/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <SharedPreview/SharedPreviewRendererContext.h>
#include <SharedPreview/SharedPreviewRendererData.h>
#include <SharedPreview/SharedPreviewRendererIdleState.h>
#include <SharedPreview/SharedPreviewUtils.h>

namespace AZ
{
    namespace LyIntegration
    {
        SharedPreviewRendererIdleState::SharedPreviewRendererIdleState(SharedPreviewRendererContext* context)
            : SharedPreviewRendererState(context)
        {
        }

        void SharedPreviewRendererIdleState::Start()
        {
            TickBus::Handler::BusConnect();
        }

        void SharedPreviewRendererIdleState::Stop()
        {
            TickBus::Handler::BusDisconnect();
        }

        void SharedPreviewRendererIdleState::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] ScriptTimePoint time)
        {
            PickNextThumbnail();
        }

        void SharedPreviewRendererIdleState::PickNextThumbnail()
        {
            if (!m_context->GetData()->m_thumbnailQueue.empty())
            {
                // pop the next thumbnailkey to be rendered from the queue
                m_context->GetData()->m_thumbnailKeyRendered = m_context->GetData()->m_thumbnailQueue.front();
                m_context->GetData()->m_thumbnailQueue.pop();

                // Find whether thumbnailkey contains a material asset or set a default material
                m_context->GetData()->m_materialAsset = m_context->GetData()->m_defaultMaterialAsset;
                Data::AssetId materialAssetId =
                    SharedPreviewUtils::GetAssetId(m_context->GetData()->m_thumbnailKeyRendered, RPI::MaterialAsset::RTTI_Type());
                if (materialAssetId.IsValid())
                {
                    if (m_context->GetData()->m_assetsToLoad.emplace(materialAssetId).second)
                    {
                        m_context->GetData()->m_materialAsset.Create(materialAssetId);
                        m_context->GetData()->m_materialAsset.QueueLoad();
                    }
                }

                // Find whether thumbnailkey contains a model asset or set a default model
                m_context->GetData()->m_modelAsset = m_context->GetData()->m_defaultModelAsset;
                Data::AssetId modelAssetId =
                    SharedPreviewUtils::GetAssetId(m_context->GetData()->m_thumbnailKeyRendered, RPI::ModelAsset::RTTI_Type());
                if (modelAssetId.IsValid())
                {
                    if (m_context->GetData()->m_assetsToLoad.emplace(modelAssetId).second)
                    {
                        m_context->GetData()->m_modelAsset.Create(modelAssetId);
                        m_context->GetData()->m_modelAsset.QueueLoad();
                    }
                }

                m_context->SetState(State::Load);
            }
        }
    } // namespace LyIntegration
} // namespace AZ
