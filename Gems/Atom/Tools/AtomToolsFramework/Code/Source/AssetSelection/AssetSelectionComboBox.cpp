/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <AtomToolsFramework/AssetSelection/AssetSelectionComboBox.h>
#include <AtomToolsFramework/Util/Util.h>

#include <QAbstractItemView>
#include <QFileInfo>

namespace AtomToolsFramework
{
    AssetSelectionComboBox::AssetSelectionComboBox(const AZStd::function<bool(const AZ::Data::AssetInfo&)>& filterCallback, QWidget* parent)
        : QComboBox(parent)
    {
        QSignalBlocker signalBlocker(this);

        setDuplicatesEnabled(true);
        setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy::AdjustToContents);
        view()->setMinimumWidth(200);

        connect(
            this, static_cast<void (QComboBox::*)(const int)>(&QComboBox::currentIndexChanged), this,
            [this](const int index)
            {
                emit AssetSelected(AZ::Data::AssetId::CreateString(itemData(index).toString().toUtf8().constData()));
            });

        SetFilterCallback(filterCallback);

        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
    }

    AssetSelectionComboBox::~AssetSelectionComboBox()
    {
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
    }

    void AssetSelectionComboBox::Reset()
    {
        clear();

        if (m_filterCallback)
        {
            AZ::Data::AssetCatalogRequests::AssetEnumerationCB enumerateCB =
                [&]([[maybe_unused]] const AZ::Data::AssetId id, const AZ::Data::AssetInfo& assetInfo)
            {
                if (m_filterCallback(assetInfo))
                {
                    addItem(
                        GetDisplayNameFromPath(AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetInfo.m_assetId).c_str()),
                        QVariant(QString(assetInfo.m_assetId.ToString<AZStd::string>().c_str())));
                }
            };

            AZ::Data::AssetCatalogRequestBus::Broadcast(
                &AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, enumerateCB, nullptr);

            model()->sort(0, Qt::AscendingOrder);
        }

        setCurrentIndex(0);
    }

    void AssetSelectionComboBox::SetFilterCallback(const AZStd::function<bool(const AZ::Data::AssetInfo&)>& filterCallback)
    {
        m_filterCallback = filterCallback;
        Reset();
    }

    void AssetSelectionComboBox::SelectAsset(const AZ::Data::AssetId& assetId)
    {
        setCurrentIndex(findData(QVariant(QString(assetId.ToString<AZStd::string>().c_str()))));
    }

    void AssetSelectionComboBox::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetCatalogRequestBus::Broadcast(
            [this, assetId](AZ::Data::AssetCatalogRequests* assetCatalogRequests)
            {
                AZ::Data::AssetInfo assetInfo = assetCatalogRequests->GetAssetInfoById(assetId);
                if (m_filterCallback && m_filterCallback(assetInfo))
                {
                    addItem(
                        GetDisplayNameFromPath(AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetInfo.m_assetId).c_str()),
                        QVariant(QString(assetInfo.m_assetId.ToString<AZStd::string>().c_str())));
                }
                model()->sort(0, Qt::AscendingOrder);
            });
    }

    void AssetSelectionComboBox::OnCatalogAssetRemoved(
        [[maybe_unused]] const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo)
    {
        if (m_filterCallback && m_filterCallback(assetInfo))
        {
            int index = findData(QVariant(QString(assetInfo.m_assetId.ToString<AZStd::string>().c_str())));
            if (index >= 0 && index < count())
            {
                removeItem(index);
            }
        }
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/AssetSelection/moc_AssetSelectionComboBox.cpp>
