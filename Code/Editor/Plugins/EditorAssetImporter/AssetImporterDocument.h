/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

/////////////////////////////////////////////////////////////////////////////
//
// Asset Importer Document hosts FBX back-end data storage and access,
// loading and saving APIs.
//
/////////////////////////////////////////////////////////////////////////////

#include <string>
#include <AzFramework/StringFunc/StringFunc.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SaveUtilities/AsyncSaveRunner.h>

class QWidget;

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class ISceneNodeGroup;
            class IMeshGroup;
            class ISkeletonGroup;
            class ISkinGroup;
            class IAnimationGroup;
            class IMaterialRule;
            class IActorGroup;
            class IEFXMotionGroup;
        }
    }
}

class AssetImporterDocument
{
public:
    AssetImporterDocument();
    virtual ~AssetImporterDocument() = default;

    bool LoadScene(const AZStd::string& sceneFullPath);
    void SaveScene(AZStd::shared_ptr<AZ::ActionOutput>& output, AZ::SaveCompleteCallback onSaveComplete);
    void ClearScene();

    AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& GetScene();
    
protected:
    void SaveManifest();

    AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene> m_scene;
    AZStd::shared_ptr<AZ::AsyncSaveRunner> m_saveRunner;
};
