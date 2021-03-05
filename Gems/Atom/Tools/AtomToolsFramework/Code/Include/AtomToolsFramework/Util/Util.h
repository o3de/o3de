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

#include <AzCore/PlatformDef.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/std/containers/vector.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QFileInfo>
#include <QString>
#include <QStringList>
AZ_POP_DISABLE_WARNING

namespace AtomToolsFramework
{
    QFileInfo GetSaveFileInfo(const QString& initialPath);
    QFileInfo GetOpenFileInfo(const AZStd::vector<AZ::Data::AssetType>& assetTypes);
    QFileInfo GetUniqueFileInfo(const QString& initialPath);
    QFileInfo GetDuplicationFileInfo(const QString& initialPath);
    bool LaunchTool(const QString& baseName, const QString& extension, const QStringList& arguments);
}
