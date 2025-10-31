/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/Events/SceneSerializationBus.h>
#include <MCore/Source/StringConversions.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            template<typename T, typename MetaDataType>
            bool MetaDataRule::SaveMetaDataToFile(const AZStd::string& sourceAssetFilename, const AZStd::string& groupName, const MetaDataType& metaData)
            {
                AZStd::string outResult;
                return SaveMetaDataToFile<T>(sourceAssetFilename, groupName, metaData, outResult);
            }

            template<typename T, typename MetaDataType>
            bool MetaDataRule::SaveMetaDataToFile(const AZStd::string& sourceAssetFilename, const AZStd::string& groupName, const MetaDataType& metaData, AZStd::string& outResult)
            {
                namespace SceneEvents = AZ::SceneAPI::Events;
                
                AZ_TraceContext("Meta data", sourceAssetFilename);

                if (sourceAssetFilename.empty())
                {
                    outResult = "Source asset filename is empty.";
                    AZ_Error("EMotionFX", false, outResult.c_str());
                    return false;
                }

                // Load the manifest from disk.
                AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene> scene;
                SceneEvents::SceneSerializationBus::BroadcastResult(scene, &SceneEvents::SceneSerializationBus::Events::LoadScene, sourceAssetFilename, AZ::Uuid::CreateNull(), "");
                if (!scene)
                {
                    outResult = "Unable to save meta data to manifest due to failed scene loading.";
                    AZ_Error("EMotionFX", false, outResult.c_str());
                    return false;
                }

                AZ::SceneAPI::Containers::SceneManifest& manifest = scene->GetManifest();
                auto values = manifest.GetValueStorage();
                auto groupView = AZ::SceneAPI::Containers::MakeDerivedFilterView<T>(values);
                for (T& group : groupView)
                {
                    // Non-case sensitive group name comparison. Product filenames are lower case only and might mismatch casing of the entered group name.
                    if (AzFramework::StringFunc::Equal(group.GetName().c_str(), groupName.c_str()))
                    {
                        SaveMetaData(*scene, group, metaData);
                    }
                }
  
                const AZStd::string& manifestFilename = scene->GetManifestFilename();


                const bool fileExisted = AZ::IO::FileIOBase::GetInstance()->Exists(manifestFilename.c_str());

                // Source Control: Checkout file.
                if (fileExisted)
                {
                    using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;
                    bool checkoutResult = false;
                    ApplicationBus::BroadcastResult(checkoutResult, &ApplicationBus::Events::RequestEditForFileBlocking, manifestFilename.c_str(), "Checking out manifest from source control.", [](int&, int&) {});
                    if (!checkoutResult)
                    {
                        outResult = AZStd::string::format("Cannot check out file '%s' from source control.", manifestFilename.c_str());
                        AZ_Error("EMotionFX", false, "%s", outResult.c_str());
                        return false;
                    }
                }

                const bool saveResult = manifest.SaveToFile(manifestFilename.c_str());

                // Source Control: Add file in case it did not exist before (when saving it the first time).
                if (saveResult && !fileExisted)
                {
                    using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;
                    bool checkoutResult = false;
                    ApplicationBus::BroadcastResult(checkoutResult, &ApplicationBus::Events::RequestEditForFileBlocking, manifestFilename.c_str(), "Adding manifest to source control.", [](int&, int&) {});
                    if (!checkoutResult)
                    {
                        outResult = AZStd::string::format("Cannot add file '%s' to source control.", manifestFilename.c_str());
                        AZ_Error("EMotionFX", checkoutResult, "%s", outResult.c_str());
                        return false;
                    }
                }

                return saveResult;
            }
        } // Rule
    } // Pipeline
} // EMotionFX
