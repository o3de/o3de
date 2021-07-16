/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        void OnPlatformSelectionChanged(
            const AzFramework::PlatformFlags& selectedPlatforms,
            const AzFramework::PlatformFlags& partiallySelectedPlatforms);

        QSharedPointer<Ui::EditSeedDialog> m_ui;
    };

} // namespace AssetBundler
