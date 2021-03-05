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

#pragma once

#include <AzCore/std/string/string.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include "AssetBuilderInfo.h"
#include <QCoreApplication>

struct IBuilderApplication
{
    AZ_RTTI(IBuilderApplication, "{FEDD188E-D5FF-4852-B945-F82F7CC1CA5F}");

    IBuilderApplication() = default;
    virtual ~IBuilderApplication() = default;

    virtual void InitializeBuilderComponents() = 0;

    AZ_DISABLE_COPY_MOVE(IBuilderApplication);
};

class AssetBuilderApplication
    : public AzToolsFramework::ToolsApplication
    , public IBuilderApplication
{
public:
    AssetBuilderApplication(int* argc, char*** argv);
    ~AssetBuilderApplication();

    AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    void RegisterCoreComponents() override;
    void StartCommon(AZ::Entity* systemEntity) override;

    bool IsInDebugMode() const;

    bool GetOptionalAppRootArg(char destinationRootArgBuffer[], size_t destinationRootArgBufferSize) const;

    void InitializeBuilderComponents() override;

private:
    void InstallCtrlHandler();

    QCoreApplication m_qtApplication;
};
