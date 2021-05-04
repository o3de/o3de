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

#if !defined(Q_MOC_RUN)
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzToolsFramework/Asset/AssetSeedManager.h>

#include <QDialog>
#include <QSharedPointer>
#endif

namespace Ui
{
    class EditSeedDialog;
}

namespace AssetBundler
{
    class EditSeedDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        explicit EditSeedDialog(
            QWidget* parent,
            const AzFramework::PlatformFlags& enabledPlatforms,
            const AzFramework::PlatformFlags& selectedPlatforms,
            const AzFramework::PlatformFlags& partiallySelectedPlatforms = AzFramework::PlatformFlags::Platform_NONE);
        virtual ~EditSeedDialog() {}

        AzFramework::PlatformFlags GetPlatformFlags();
        AzFramework::PlatformFlags GetPartiallySelectedPlatformFlags();

    private:
        void OnPlatformSelectionChanged(const AzFramework::PlatformFlags& selectedPlatforms, const AzFramework::PlatformFlags& partiallySelectedPlatforms);

        QSharedPointer<Ui::EditSeedDialog> m_ui;
    };

} // namespace AssetBundler
