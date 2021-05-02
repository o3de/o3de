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
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Asset/AssetBundler.h>

#include <QDialog>
#include <QSharedPointer>
#endif

namespace Ui
{
    class GenerateBundlesModal;
}

namespace AssetBundler
{
    class AssetListTabWidget;

    class GenerateBundlesModal
        : public QDialog
    {
        Q_OBJECT

    public:
        explicit GenerateBundlesModal(
            QWidget* parent,
            const AZStd::string& assetListFileAbsolutePath,
            const AZStd::string& defaultBundleDirectory,
            const AZStd::string& defaultBundleSettingsDirectory,
            AssetListTabWidget* assetListTabWidget);
        virtual ~GenerateBundlesModal();

    private:
        void OnOutputBundleLocationBrowseButtonPressed();

        void OnBundleSettingsBrowseButtonPressed();

        bool LoadBundleSettingsValues(const AZStd::string& absoluteBundleSettingsFilePath);

        void UpdateBundleSettingsDisplayName(const AZStd::string& absoluteBundleSettingsFilePath);

        void OnBundleSettingsSaveButtonPressed();

        void OnMaxBundleSizeChanged();

        void OnBundleVersionChanged();

        void OnGenerateBundlesButtonPressed();

        QSharedPointer<Ui::GenerateBundlesModal> m_ui;

        AssetListTabWidget* m_assetListTabWidget = nullptr;

        AZStd::string m_assetListFileAbsolutePath;
        AZStd::string m_defaultBundleDirectory;
        AZStd::string m_defaultBundleSettingsDirectory;
        AZStd::string m_platformName;

        AzToolsFramework::AssetBundleSettings m_bundleSettings;
    };

} // namespace AssetBundler
