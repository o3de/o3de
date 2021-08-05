/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Platform/PlatformDefaults.h>

#include <QCheckBox>
#include <QSharedPointer>
#include <QWidget>
#endif

namespace Ui
{
    class PlatformSelectionWidget;
}

namespace AssetBundler
{
    class PlatformSelectionWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit PlatformSelectionWidget(QWidget* parent);
        virtual ~PlatformSelectionWidget() {}

        void Init(const AzFramework::PlatformFlags& enabledPlatforms, const QString& disabledPatformMessageOverride = "");

        void SetSelectedPlatforms(
            const AzFramework::PlatformFlags& selectedPlatforms,
            const AzFramework::PlatformFlags& partiallySelectedPlatforms);

        AzFramework::PlatformFlags GetSelectedPlatforms();
        AzFramework::PlatformFlags GetPartiallySelectedPlatforms();

    Q_SIGNALS:
        void PlatformsSelected(AzFramework::PlatformFlags selectedPlatforms, AzFramework::PlatformFlags partiallySelectedPlatforms);

    private:
        void OnPlatformSelectionChanged();

        QSharedPointer<Ui::PlatformSelectionWidget> m_ui;
        AZStd::unique_ptr<AzFramework::PlatformHelper> m_platformHelper;

        QVector<QSharedPointer<QCheckBox>> m_platformCheckBoxes;
        AZStd::vector<AzFramework::PlatformFlags> m_platformList;

        AzFramework::PlatformFlags m_selectedPlatforms;
        AzFramework::PlatformFlags m_partiallySelectedPlatforms;
    };
} // namespace AssetBundler
