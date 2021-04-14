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

#include "Atom/RPI.Reflect/Model/ModelAsset.h"

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Transform.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <Thumbnails/Thumbnail.h>

namespace AzFramework
{
    class Scene;
}

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            //! ThumbnailRendererData encapsulates all data used by thumbnail renderer and caches assets
            struct ThumbnailRendererData final
            {
                static constexpr const char* LightingPresetPath = "lightingpresets/thumbnail.lightingpreset.azasset";
                static constexpr const char* DefaultModelPath = "materialeditor/viewportmodels/quadsphere.azmodel";
                static constexpr const char* DefaultMaterialPath = "materials/basic_grey.azmaterial";

                RPI::ScenePtr m_scene;
                AZStd::string m_sceneName = "Material Thumbnail Scene";
                AZStd::string m_pipelineName = "Material Thumbnail Pipeline";
                AzFramework::Scene* m_frameworkScene = nullptr;
                RPI::RenderPipelinePtr m_renderPipeline;
                AZStd::unique_ptr<AzFramework::EntityContext> m_entityContext;
                AZStd::vector<AZStd::string> m_passHierarchy;

                RPI::ViewPtr m_view = nullptr;
                Entity* m_modelEntity = nullptr;

                double m_simulateTime = 0.0f;
                float m_deltaTime = 0.0f;

                //! Incoming thumbnail requests are appended to this queue and processed one at a time in OnTick function.
                AZStd::queue<AzToolsFramework::Thumbnailer::SharedThumbnailKey> m_thumbnailQueue;
                //! Current thumbnail key being rendered.
                AzToolsFramework::Thumbnailer::SharedThumbnailKey m_thumbnailKeyRendered;

                Data::Asset<RPI::AnyAsset> m_lightingPresetAsset;

                Data::Asset<RPI::ModelAsset> m_defaultModelAsset;
                //! Model asset about to be rendered
                Data::Asset<RPI::ModelAsset> m_modelAsset;

                Data::Asset<RPI::MaterialAsset> m_defaultMaterialAsset;
                //! Material asset about to be rendered
                Data::Asset<RPI::MaterialAsset> m_materialAsset;

                AZStd::unordered_set<Data::AssetId> m_assetsToLoad;
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
