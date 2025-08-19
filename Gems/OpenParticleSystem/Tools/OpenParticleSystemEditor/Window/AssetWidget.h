/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/string/string.h>
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <AzCore/Asset/AssetCommon.h>
#endif
#include <QWidget>

namespace OpenParticleSystemEditor
{
    using AssetChangeCB = AZStd::function<void(const char*)>;
    class AssetWidget
        : public QWidget
    {
        Q_OBJECT;

    public:
        AssetWidget(const AZStd::string& label, const AZ::Data::AssetType& assetType, const AZStd::string& extention, QWidget* parent = nullptr);
        ~AssetWidget() override {}
        void SetAssetPath(const AZStd::string& path);
        void Clear();
        
        void OnAssetSelectionChanged(AssetChangeCB funcPtr)
        {
            m_AssetSelectionChanged = funcPtr;
        }

    private:
        void PopupAssetPicker();

        AZStd::string m_title;
        AZ::Data::AssetType m_assetType;
        AZStd::string m_extention;
        AzQtComponents::BrowseEdit* m_assetPathBrowseEdit;
        AssetChangeCB m_AssetSelectionChanged;
    };
} // namespace OpenParticleSystemEditor
