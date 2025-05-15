/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <SceneAPI/SDKWrapper/SceneWrapper.h>
#include <SceneAPI/SceneCore/Import/SceneImportSettings.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

struct aiScene;

namespace AZ
{
    namespace AssImpSDKWrapper
    {
        class AssImpSceneWrapper : public SDKScene::SceneWrapperBase
        {
        public:
            AZ_RTTI(AssImpSceneWrapper, "{43A61F62-DCD4-4132-B80B-F2FBC80740BC}", SDKScene::SceneWrapperBase);
            AssImpSceneWrapper();
            AssImpSceneWrapper(aiScene* aiScene);
            ~AssImpSceneWrapper() override = default;

            bool LoadSceneFromFile(const char* fileName, const AZ::SceneAPI::SceneImportSettings& importSettings) override;
            bool LoadSceneFromFile(const AZStd::string& fileName, const AZ::SceneAPI::SceneImportSettings& importSettings) override;

            const std::shared_ptr<SDKNode::NodeWrapper> GetRootNode() const override;
            std::shared_ptr<SDKNode::NodeWrapper> GetRootNode() override;
            virtual const aiScene* GetAssImpScene() const;
            void Clear() override;
            void CalculateAABBandVertices(const aiScene* scene, aiAABB& aabb, uint32_t& vertices);

            AZStd::pair<AxisVector, int32_t> GetUpVectorAndSign() const override;
            AZStd::pair<AxisVector, int32_t> GetFrontVectorAndSign() const override;
            AZStd::pair<AxisVector, int32_t> GetRightVectorAndSign() const override;
            AZStd::optional<SceneAPI::DataTypes::MatrixType> UseForcedRootTransform() const override;
            float GetUnitSizeInMeters() const override;

            AZStd::string GetSceneFileName() const { return m_sceneFileName; }
            AZ::Aabb GetAABB() const override;
            uint32_t GetVerticesCount() const override { return m_vertices; }

            bool GetExtractEmbeddedTextures() const { return m_extractEmbeddedTextures; }
        protected:
            const aiScene* m_assImpScene = nullptr;
            AZStd::unique_ptr<Assimp::Importer> m_importer;

            // FBX SDK automatically resolved relative paths to textures based on the current file location.
            // AssImp does not, so it needs to be specifically handled.
            AZStd::string m_sceneFileName;
            aiAABB m_aabb;
            uint32_t m_vertices;
            bool m_extractEmbeddedTextures;
        };

    } // namespace AssImpSDKWrapper
} // namespace AZ
