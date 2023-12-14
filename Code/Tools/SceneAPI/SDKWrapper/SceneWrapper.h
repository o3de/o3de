/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SDKWrapper/NodeWrapper.h>
#include <SceneAPI/SceneCore/Import/SceneImportSettings.h>

struct aiScene;

namespace AZ
{
    namespace SDKScene
    {
        class SceneWrapperBase
        {
        public:
            AZ_RTTI(SceneWrapperBase, "{703CD344-2C75-4F30-8CE2-6BDEF2511AFD}");
            virtual ~SceneWrapperBase() = default;

            virtual bool LoadSceneFromFile(const char* fileName, const SceneAPI::SceneImportSettings& importSettings = {});
            virtual bool LoadSceneFromFile(const AZStd::string& fileName, const SceneAPI::SceneImportSettings& importSettings = {});

            virtual const std::shared_ptr<SDKNode::NodeWrapper> GetRootNode() const;
            virtual std::shared_ptr<SDKNode::NodeWrapper> GetRootNode();

            virtual void Clear();

            static const char* s_defaultSceneName;
        };

    } //namespace Scene
} //namespace AZ
