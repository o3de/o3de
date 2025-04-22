/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Debug/Trace.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <assimp/postprocess.h>

#if AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <stdlib.h>
#endif // AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL

namespace AZ
{
    namespace AssImpSDKWrapper
    {
        static constexpr const char s_UseSkeletonBoneContainerKey[] = "/O3DE/Preferences/SceneAPI/UseSkeletonBoneContainer";

        AssImpSceneWrapper::AssImpSceneWrapper()
            : m_assImpScene(nullptr)
            , m_importer(AZStd::make_unique<Assimp::Importer>())
        {
        }
        AssImpSceneWrapper::AssImpSceneWrapper(aiScene* aiScene)
            : m_assImpScene(aiScene)
            , m_importer(AZStd::make_unique<Assimp::Importer>())
        {
        }

#if AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL
        void signal_handler([[maybe_unused]] int signal) 
        {
            AZ_TracePrintf(
                SceneAPI::Utilities::ErrorWindow,
                "Failed to import scene with Asset Importer library. An %s has occurred in the library, this scene file cannot be parsed by the library.",
                signal == SIGABRT ? "assert" : "unknown error");
        }
#endif // AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL

        bool AssImpSceneWrapper::LoadSceneFromFile(const char* fileName, const AZ::SceneAPI::SceneImportSettings& importSettings)
        {
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "AssImpSceneWrapper::LoadSceneFromFile %s", fileName);
            AZ_TraceContext("Filename", fileName);

#if AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL
            // Turn off the abort popup because it can disrupt automation.
            // AssImp calls abort when asserts are enabled, and an assert is encountered.
#ifdef _WRITE_ABORT_MSG
            _set_abort_behavior(0, _WRITE_ABORT_MSG);
#endif // #ifdef _WRITE_ABORT_MSG
            // Instead, capture any calls to abort with a signal handler, and report them.
            auto previous_handler = std::signal(SIGABRT, signal_handler);
#endif // AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL

            bool useSkeletonBoneContainer = false;
            if (AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry)
            {
                settingsRegistry->Get(useSkeletonBoneContainer, s_UseSkeletonBoneContainerKey);
            }

            // aiProcess_JoinIdenticalVertices is not enabled because O3DE has a mesh optimizer that also does this,
            // this flag is disabled to keep AssImp output similar to FBX SDK to reduce downstream bugs for the initial AssImp release.
            // There's currently a minimum of properties and flags set to maximize compatibility with the existing node graph.
            unsigned int importFlags =
                aiProcess_Triangulate                                               // Triangulates all faces of all meshes
                | static_cast<unsigned long>(aiProcess_GenBoundingBoxes)            // Generate bounding boxes
                | aiProcess_GenNormals                                              // Generate normals for meshes
                | (importSettings.m_optimizeScene ? aiProcess_OptimizeGraph : 0)    // Merge excess scene nodes together
                | (importSettings.m_optimizeMeshes ? aiProcess_OptimizeMeshes : 0)  // Combines meshes in the scene together
                ;

            // aiProcess_LimitBoneWeights is not enabled because it will remove bones which are not associated with a mesh.
            // This results in the loss of the offset matrix data for nodes without a mesh which is required for the Transform Importer.
            m_importer->SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
            m_importer->SetPropertyBool(AI_CONFIG_IMPORT_FBX_OPTIMIZE_EMPTY_ANIMATION_CURVES, false);
            m_importer->SetPropertyBool(AI_CONFIG_FBX_USE_SKELETON_BONE_CONTAINER, useSkeletonBoneContainer);
            // The remove empty bones flag is on by default, but doesn't do anything internal to AssImp right now.
            // This is here as a bread crumb to save others times investigating issues with empty bones.
            // m_importer.SetPropertyBool(AI_CONFIG_IMPORT_REMOVE_EMPTY_BONES, false);
            m_sceneFileName = fileName;
            m_assImpScene = m_importer->ReadFile(fileName, importFlags);

#if AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL
            // Reset abort behavior for anything else that may call abort.
            std::signal(SIGABRT, previous_handler);
#ifdef _WRITE_ABORT_MSG
            _set_abort_behavior(1, _WRITE_ABORT_MSG);
#endif // #ifdef _WRITE_ABORT_MSG
#endif // AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL

            if (!m_assImpScene)
            {
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Failed to import Asset Importer Scene. Error returned: %s", m_importer->GetErrorString());
                return false;
            }

            CalculateAABBandVertices(m_assImpScene, m_aabb, m_vertices);

            return true;
        }

        bool AssImpSceneWrapper::LoadSceneFromFile(const AZStd::string& fileName, const AZ::SceneAPI::SceneImportSettings& importSettings)
        {
            return LoadSceneFromFile(fileName.c_str(), importSettings);
        }

        const std::shared_ptr<SDKNode::NodeWrapper> AssImpSceneWrapper::GetRootNode() const
        {
            return std::shared_ptr<SDKNode::NodeWrapper>(new AssImpNodeWrapper(m_assImpScene->mRootNode));
        }

        std::shared_ptr<SDKNode::NodeWrapper> AssImpSceneWrapper::GetRootNode()
        {
            return std::shared_ptr<SDKNode::NodeWrapper>(new AssImpNodeWrapper(m_assImpScene->mRootNode));
        }

        void AssImpSceneWrapper::CalculateAABBandVertices(const aiScene* scene, aiAABB& aabb, uint32_t& vertices)
        {
            if (scene->HasMeshes())
            {
                aabb = scene->mMeshes[0]->mAABB;
                vertices = scene->mMeshes[0]->mNumVertices;

                for (unsigned int i = 1; i < scene->mNumMeshes; ++i)
                {
                    const aiAABB& thisAabb = scene->mMeshes[i]->mAABB;
                    if (thisAabb.mMin.x < aabb.mMin.x)
                        aabb.mMin.x = thisAabb.mMin.x;
                    if (thisAabb.mMin.y < aabb.mMin.y)
                        aabb.mMin.y = thisAabb.mMin.y;
                    if (thisAabb.mMin.z < aabb.mMin.z)
                        aabb.mMin.z = thisAabb.mMin.z;
                    if (thisAabb.mMax.x > aabb.mMax.x)
                        aabb.mMax.x = thisAabb.mMax.x;
                    if (thisAabb.mMax.y > aabb.mMax.y)
                        aabb.mMax.y = thisAabb.mMax.y;
                    if (thisAabb.mMax.z > aabb.mMax.z)
                        aabb.mMax.z = thisAabb.mMax.z;
                    vertices += scene->mMeshes[i]->mNumVertices;
                }
            }
        }

        void AssImpSceneWrapper::Clear()
        {
            if(m_importer)
            {
                m_importer->FreeScene();
                m_importer = AZStd::make_unique<Assimp::Importer>();
            }
        }

        const aiScene* AssImpSceneWrapper::GetAssImpScene() const
        {
            return m_assImpScene;
        }

        AZStd::pair<AssImpSceneWrapper::AxisVector, int32_t> AssImpSceneWrapper::GetUpVectorAndSign() const
        {
            AZStd::pair<AssImpSceneWrapper::AxisVector, int32_t> result(AxisVector::Z, 1);
            int32_t upVectorRead(static_cast<int32_t>(result.first));
            m_assImpScene->mMetaData->Get("UpAxis", upVectorRead);
            m_assImpScene->mMetaData->Get("UpAxisSign", result.second);
            result.first = static_cast<AssImpSceneWrapper::AxisVector>(upVectorRead);
            return result;
        }

        AZStd::pair<AssImpSceneWrapper::AxisVector, int32_t> AssImpSceneWrapper::GetFrontVectorAndSign() const
        {
            AZStd::pair<AssImpSceneWrapper::AxisVector, int32_t> result(AxisVector::Y, 1);
            int32_t frontVectorRead(static_cast<int32_t>(result.first));
            m_assImpScene->mMetaData->Get("FrontAxis", frontVectorRead);
            m_assImpScene->mMetaData->Get("FrontAxisSign", result.second);
            result.first = static_cast<AssImpSceneWrapper::AxisVector>(frontVectorRead);
            return result;
        }
    }//namespace AssImpSDKWrapper

} // namespace AZ
