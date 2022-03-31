/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Debug/Trace.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#if AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <stdlib.h>
#endif // AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL

#include <AzCore/std/string/conversions.h>

namespace AZ
{
    namespace AssImpSDKWrapper
    {
        AssImpSceneWrapper::AssImpSceneWrapper()
        {
            m_importer = AZStd::make_unique<Assimp::Importer>();
        }

        AssImpSceneWrapper::AssImpSceneWrapper(aiScene* aiScene)
            : m_assImpScene(aiScene)
        {
            m_importer = AZStd::make_unique<Assimp::Importer>();
        }

#if AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL
        void signal_handler([[maybe_unused]] int signal) 
        {
            AZ_TracePrintf(
                SceneAPI::Utilities::ErrorWindow,
                "Failed to import scene with Asset Importer library. An %s has occured in the library, this scene file cannot be parsed by the library.",
                signal == SIGABRT ? "assert" : "unknown error");
        }
#endif // AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL

        bool AssImpSceneWrapper::LoadSceneFromFile(const char* fileName)
        {
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "AssImpSceneWrapper::LoadSceneFromFile %s", fileName);
            AZ_TraceContext("Filename", fileName);

            unsigned int defaultProcessingFlags =
                aiProcess_Triangulate //Triangulates all faces of all meshes
                | aiProcess_GenNormals; //Generate normals for meshes

            unsigned int processingFlags = defaultProcessingFlags;

#if AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL
            // Turn off the abort popup because it can disrupt automation.
            // AssImp calls abort when asserts are enabled, and an assert is encountered.
#ifdef _WRITE_ABORT_MSG
            _set_abort_behavior(0, _WRITE_ABORT_MSG);
#endif // #ifdef _WRITE_ABORT_MSG
            // Instead, capture any calls to abort with a signal handler, and report them.
            auto previous_handler = std::signal(SIGABRT, signal_handler);
#endif // AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL

            if (m_importFlagsMap.empty())
            {
                // aiProcess_JoinIdenticalVertices is not enabled because O3DE has a mesh optimizer that also does this,
                // this flag is disabled to keep AssImp output similar to FBX SDK to reduce downstream bugs for the initial AssImp release.
                // There's currently a minimum of properties and flags set to maximize compatibility with the existing node graph.

                // aiProcess_LimitBoneWeights is not enabled because it will remove bones which are not associated with a mesh.
                // This results in the loss of the offset matrix data for nodes without a mesh which is required for the Transform Importer.

                m_importer->SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
                m_importer->SetPropertyBool(AI_CONFIG_IMPORT_FBX_OPTIMIZE_EMPTY_ANIMATION_CURVES, false);
                // The remove empty bones flag is on by default, but doesn't do anything internal to AssImp right now.
                // This is here as a bread crumb to save others times investigating issues with empty bones.
                // m_importer.SetPropertyBool(AI_CONFIG_IMPORT_REMOVE_EMPTY_BONES, false);
            }
            else
            {
                // update m_importer flags via SetPropertyXYZ using the new m_importFlagsMap
                // OR
                // use the ProcessFlags value to set the processingFlags
                for (const auto& flag : m_importFlagsMap)
                {
                    if (flag.first == "ProcessFlags")
                    {
                        if (flag.second.is<AZ::s64>())
                        {
                            AZ::s64 value = {};
                            AZStd::any_numeric_cast(&flag.second, value);
                            processingFlags = aznumeric_cast<unsigned int>(value);
                        }
                        else if (flag.second.is<AZ::s64>())
                        {
                            AZ::u64 value = {};
                            AZStd::any_numeric_cast(&flag.second, value);
                            processingFlags = aznumeric_cast<unsigned int>(value);
                        }
                        else if (flag.second.is<AZStd::string>())
                        {
                            const AZStd::string* value = AZStd::any_cast<AZStd::string>(&flag.second);
                            // is hex string?
                            if(value->length() > 2 && (*value)[0] != '0' && (*value)[1] == 'x')
                            {
                                processingFlags = AZStd::stoul(*value, nullptr, 16);
                            }
                            else
                            {
                                processingFlags = AZStd::stoul(*value, nullptr, 10);
                            }
                        }
                    }
                    else if (flag.second.is<bool>())
                    {
                        bool value = {};
                        AZStd::any_numeric_cast(&flag.second, value);
                        m_importer->SetPropertyBool(flag.first.c_str(), value);
                    }
                    else if (flag.second.is<AZ::s64>())
                    {
                        AZ::s64 value = {};
                        AZStd::any_numeric_cast(&flag.second, value);
                        m_importer->SetPropertyInteger(flag.first.c_str(), aznumeric_cast<int>(value));
                    }
                    else if (flag.second.is<AZ::s64>())
                    {
                        AZ::u64 value = {};
                        AZStd::any_numeric_cast(&flag.second, value);
                        m_importer->SetPropertyInteger(flag.first.c_str(), aznumeric_cast<int>(value));
                    }
                    else if (flag.second.is<double>())
                    {
                        double value = {};
                        AZStd::any_numeric_cast(&flag.second, value);
                        m_importer->SetPropertyFloat(flag.first.c_str(), aznumeric_cast<ai_real>(value));
                    }
                    else if (flag.second.is<AZStd::string>())
                    {
                        std::string value{ AZStd::any_cast<AZStd::string>(&flag.second)->c_str() };
                        m_importer->SetPropertyString(flag.first.c_str(), value);
                    }
                }
            }
            m_sceneFileName = fileName;
            m_assImpScene = m_importer->ReadFile(fileName, processingFlags);

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

            return true;
        }

        bool AssImpSceneWrapper::LoadSceneFromFile(const AZStd::string& fileName)
        {
            return LoadSceneFromFile(fileName.c_str());
        }

        const std::shared_ptr<SDKNode::NodeWrapper> AssImpSceneWrapper::GetRootNode() const
        {
            return std::shared_ptr<SDKNode::NodeWrapper>(new AssImpNodeWrapper(m_assImpScene->mRootNode));
        }
        std::shared_ptr<SDKNode::NodeWrapper> AssImpSceneWrapper::GetRootNode()
        {
            return std::shared_ptr<SDKNode::NodeWrapper>(new AssImpNodeWrapper(m_assImpScene->mRootNode));
        }

        void AssImpSceneWrapper::Clear()
        {
            m_importer->FreeScene();
            m_importer = AZStd::make_unique<Assimp::Importer>();
            // Do not clear the m_importFlagsMap since it will be filled out during the pre-import phase
        }

        void AssImpSceneWrapper::SetImportFlags(FlagsMap&& flagsMap)
        {
            m_importFlagsMap = AZStd::move(flagsMap);
        }

        const AssImpSceneWrapper::FlagsMap& AssImpSceneWrapper::GetImportFlags() const
        {
            return m_importFlagsMap;
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
