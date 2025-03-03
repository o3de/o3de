/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzQtComponents/Application/AzQtApplication.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzToolsFramework/Application/ToolsApplication.h>

namespace ScriptCanvas
{
    class ScriptCanvasQtApplication : public AzQtComponents::AzQtApplication
    {
    public:
        using Base = AzQtComponents::AzQtApplication;

        ScriptCanvasQtApplication(int& argc, char** argv);
        virtual ~ScriptCanvasQtApplication() = default;
    };

    class ScriptCanvasToolsApplication
        : public AzToolsFramework::ToolsApplication
        , protected AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasToolsApplication, AZ::SystemAllocator)
        AZ_TYPE_INFO(ScriptCanvasToolsApplication, "{484D42F9-30C5-4221-BF23-EDCA71726C05}");
        using Base = AzToolsFramework::ToolsApplication;

        ScriptCanvasToolsApplication(int* argc, char*** argv, AZ::ComponentApplicationSettings componentAppSettings);
        virtual ~ScriptCanvasToolsApplication() = default;

        // AzToolsFramework::ToolsApplication overrides
        void StartCommon(AZ::Entity* systemEntity) override;
        void RegisterCoreComponents() override;
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        void Destroy() override;
        // ~AzToolsFramework::ToolsApplication

        // AssetDatabaseRequestsBus::Handler overrides
        bool GetAssetDatabaseLocation(AZStd::string& result) override;
        // ~AssetDatabaseRequestsBus::Handler

    private:
        void ConnectToAssetProcessor();
    };
} // namespace ScriptCanvas
