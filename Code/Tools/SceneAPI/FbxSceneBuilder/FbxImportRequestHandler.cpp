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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>
#include <SceneAPI/SceneCore/Events/ImportEventContext.h>
#include <SceneAPI/FbxSceneBuilder/FbxImportRequestHandler.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneImporter
        {
            const char* FbxImportRequestHandler::s_extension = ".fbx";

            void FbxImportRequestHandler::Activate()
            {
                BusConnect();
            }

            void FbxImportRequestHandler::Deactivate()
            {
                BusDisconnect();
            }

            void FbxImportRequestHandler::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<FbxImportRequestHandler, SceneCore::BehaviorComponent>()->Version(1);
                }
            }

            void FbxImportRequestHandler::GetSupportedFileExtensions(AZStd::unordered_set<AZStd::string>& extensions)
            {
                extensions.insert(s_extension);
            }

            Events::LoadingResult FbxImportRequestHandler::LoadAsset(Containers::Scene& scene, const AZStd::string& path, const Uuid& guid, [[maybe_unused]] RequestingApplication requester)
            {
                if (!AzFramework::StringFunc::Path::IsExtension(path.c_str(), s_extension))
                {
                    return Events::LoadingResult::Ignored;
                }

                scene.SetSource(path, guid);

                // Push contexts
                Events::ProcessingResultCombiner contextResult;
                contextResult += Events::Process<Events::PreImportEventContext>(path);
                contextResult += Events::Process<Events::ImportEventContext>(path, scene);
                contextResult += Events::Process<Events::PostImportEventContext>(scene);

                if (contextResult.GetResult() == Events::ProcessingResult::Success)
                {
                    return Events::LoadingResult::AssetLoaded;
                }
                else
                {
                    return Events::LoadingResult::AssetFailure;
                }
            }
        } // namespace Import
    } // namespace SceneAPI
} // namespace AZ