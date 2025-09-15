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

namespace AzToolsFramework
{
    class PropertyAssetCtrl;
}

namespace OpenParticleSystemEditor
{
    using AssetChangeCB = AZStd::function<void(AZ::Data::AssetId)>;
    class AssetWidget
        : public QWidget
    {
        Q_OBJECT;

    public:
        AssetWidget(const AZStd::string& label, const AZ::Data::AssetType& assetType, QWidget* parent = nullptr);
        ~AssetWidget() override {}
        void Clear();
        void SetAssetId(const AZ::Data::AssetId& newId);

        void SetOnAssetSelectionChangedCallback(AssetChangeCB funcPtr);

    private:
        AzToolsFramework::PropertyAssetCtrl* m_assetCtrl = nullptr;
        AssetChangeCB m_AssetSelectionChanged;

    };
} // namespace OpenParticleSystemEditor
