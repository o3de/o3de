/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetImporterDocument.h>
#include <Util/PathUtil.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMaterialRule.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkeletonGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkinGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IAnimationGroup.h>
#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>
#include <SceneAPI/SceneCore/Events/SceneSerializationBus.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <IEditor.h>
#include <QFile>
#include <QWidget>
#include <QMessageBox>
#include <QPushButton>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <ActionOutput.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

AssetImporterDocument::AssetImporterDocument()
{
}

bool AssetImporterDocument::LoadScene(const AZStd::string& sceneFullPath)
{
    AZ_PROFILE_FUNCTION(Editor);
    namespace SceneEvents = AZ::SceneAPI::Events;
    SceneEvents::SceneSerializationBus::BroadcastResult(m_scene, &SceneEvents::SceneSerializationBus::Events::LoadScene, sceneFullPath, AZ::Uuid::CreateNull(), "");
    return !!m_scene;
}

void AssetImporterDocument::SaveScene(AZStd::shared_ptr<AZ::ActionOutput>& output, AZ::SaveCompleteCallback onSaveComplete)
{
    if (!m_scene)
    {
        if (output)
        {
            output->AddError("No scene file was loaded.");
        }

        if (onSaveComplete)
        {
            onSaveComplete(false);
        }

        return;
    }

    // If a save is requested, the user is going to want to see the asset re-processed, even if they didn't actually make a change.
    bool fingerprintCleared = false;
    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
        fingerprintCleared, &AzToolsFramework::AssetSystemRequestBus::Events::ClearFingerprintForAsset, m_scene->GetManifestFilename());
    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
        fingerprintCleared, &AzToolsFramework::AssetSystemRequestBus::Events::ClearFingerprintForAsset, m_scene->GetSourceFilename());

    m_saveRunner = AZStd::make_shared<AZ::AsyncSaveRunner>();

    // Add a no-op saver to put the FBX into source control. The benefit of doing it this way
    //  rather than invoking the SourceControlCommands bus directly is that we enable ourselves
    //  to have a single callback point for fbx & the manifest.
    auto fbxNoOpSaver = m_saveRunner->GenerateController();
    fbxNoOpSaver->AddSaveOperation(m_scene->GetSourceFilename(), nullptr);

    // Save the manifest
    SaveManifest();

    m_saveRunner->Run(output,
        [this, onSaveComplete](bool success)
        {
            if (onSaveComplete)
            {
                onSaveComplete(success);
            }

            m_saveRunner = nullptr;
    }, AZ::AsyncSaveRunner::ControllerOrder::Sequential);
}

void AssetImporterDocument::ClearScene()
{
    m_scene.reset();
}

void AssetImporterDocument::SaveManifest()
{
    // Create the save controller and add the save operation for the manifest job to it
    AZStd::shared_ptr<AZ::SaveOperationController> saveController = m_saveRunner->GenerateController();

    saveController->AddSaveOperation(m_scene->GetManifestFilename(),
        [this](const AZStd::string& fullPath, const AZStd::shared_ptr<AZ::ActionOutput>& actionOutput) -> bool
        {
            AZ_UNUSED(actionOutput);
            return m_scene->GetManifest().SaveToFile(fullPath.c_str());
        });
}

AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& AssetImporterDocument::GetScene()
{
    return m_scene;
}
