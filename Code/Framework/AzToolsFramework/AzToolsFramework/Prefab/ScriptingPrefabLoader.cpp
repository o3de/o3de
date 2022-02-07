/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/ScriptingPrefabLoader.h>

namespace AzToolsFramework::Prefab
{
    void ScriptingPrefabLoader::Connect(PrefabLoaderInterface* prefabLoaderInterface)
    {
        AZ_Assert(prefabLoaderInterface, "prefabLoaderInterface must not be null");

        m_prefabLoaderInterface = prefabLoaderInterface;
        PrefabLoaderScriptingBus::Handler::BusConnect();
    }

    void ScriptingPrefabLoader::Disconnect()
    {
        PrefabLoaderScriptingBus::Handler::BusDisconnect();
    }

    AZ::Outcome<AZStd::string, void> ScriptingPrefabLoader::SaveTemplateToString(TemplateId templateId)
    {
        AZStd::string json;

        if (m_prefabLoaderInterface->SaveTemplateToString(templateId, json))
        {
            return AZ::Success(json);
        }

        return AZ::Failure();
    }
} // namespace AzToolsFramework::Prefab
