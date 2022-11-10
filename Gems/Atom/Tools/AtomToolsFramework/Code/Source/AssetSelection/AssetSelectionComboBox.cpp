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
#include <AzToolsFramework/AssetBrowser/Thumbnails/ProductThumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

#include <QAbstractItemView>
#include <QFileInfo>
#include <QTimer>

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
            [this]() { emit AssetSelected(GetSelectedAsset()); });

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
        m_thumbnailKeys.clear();

        AZ::Data::AssetCatalogRequests::AssetEnumerationCB enumerateCB =
            [this]([[maybe_unused]] const AZ::Data::AssetId assetId, const AZ::Data::AssetInfo& assetInfo) { AddAsset(assetInfo); };

        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, enumerateCB, nullptr);

        model()->sort(0, Qt::AscendingOrder);

        setCurrentIndex(0);
    }

    void AssetSelectionComboBox::SetFilterCallback(const AZStd::function<bool(const AZ::Data::AssetInfo&)>& filterCallback)
    {
        m_filterCallback = filterCallback;
        Reset();
    }

    void AssetSelectionComboBox::SelectAsset(const AZ::Data::AssetId& assetId)
    {
        const QVariant assetIdItemData(assetId.ToFixedString().c_str());
        const int index = findData(assetIdItemData);
        setCurrentIndex(index);
    }

    AZ::Data::AssetId AssetSelectionComboBox::GetSelectedAsset() const
    {
        return AZ::Data::AssetId::CreateString(currentData().toString().toUtf8().constData());
    }

    AZStd::string AssetSelectionComboBox::GetSelectedAssetSourcePath() const
    {
        return AZ::RPI::AssetUtils::GetSourcePathByAssetId(GetSelectedAsset());
    }

    AZStd::string AssetSelectionComboBox::GetSelectedAssetProductPath() const
    {
        return AZ::RPI::AssetUtils::GetProductPathByAssetId(GetSelectedAsset());
    }

    void AssetSelectionComboBox::SetThumbnailsEnabled(bool enabled)
    {
        if (m_thumbnailsEnabled != enabled)
        {
            m_thumbnailKeys.clear();
            m_thumbnailsEnabled = enabled;
            for (int index = 0; index < count(); ++index)
            {
                setItemIcon(index, QIcon());
                RegisterThumbnail(AZ::Data::AssetId::CreateString(itemData(index).toString().toUtf8().constData()));
            }
        }
    }

    void AssetSelectionComboBox::SetThumbnailDelayMs(AZ::u32 delay)
    {
        m_thumbnailDelayMs = delay;
    }

    void AssetSelectionComboBox::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetCatalogRequestBus::Broadcast(
            [this, assetId](AZ::Data::AssetCatalogRequests* assetCatalogRequests)
            {
                AddAsset(assetCatalogRequests->GetAssetInfoById(assetId));
                model()->sort(0, Qt::AscendingOrder);
            });
    }

    void AssetSelectionComboBox::OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo)
    {
        if (m_filterCallback && m_filterCallback(assetInfo))
        {
            const QVariant assetIdItemData(assetId.ToFixedString().c_str());
            const int index = findData(assetIdItemData);
            removeItem(index);
        }
    }

    void AssetSelectionComboBox::AddAsset(const AZ::Data::AssetInfo& assetInfo)
    {
        if (m_filterCallback && m_filterCallback(assetInfo))
        {
            // Only add the asset if no item exists with the incoming asset ID
            const QVariant assetIdItemData(assetInfo.m_assetId.ToFixedString().c_str());
            const int index = findData(assetIdItemData);
            if (index < 0)
            {
                const AZStd::string path = AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetInfo.m_assetId);
                const AZStd::string displayName = GetDisplayNameFromPath(path);
                addItem(displayName.c_str(), assetIdItemData);
                RegisterThumbnail(assetInfo.m_assetId);
            }
        }
    }

    void AssetSelectionComboBox::RegisterThumbnail(const AZ::Data::AssetId& assetId)
    {
        if (m_thumbnailsEnabled)
        {
            AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey =
                MAKE_TKEY(AzToolsFramework::AssetBrowser::ProductThumbnailKey, assetId);
            m_thumbnailKeys[assetId] = thumbnailKey;

            connect(
                thumbnailKey.data(), &AzToolsFramework::Thumbnailer::ThumbnailKey::ThumbnailUpdatedSignal, this,
                [this, assetId]() { QueueUpdateThumbnail(assetId); });

            QueueUpdateThumbnail(assetId);
        }
    }

    void AssetSelectionComboBox::UpdateThumbnail(const AZ::Data::AssetId& assetId)
    {
        if (m_thumbnailsEnabled)
        {
            auto thumbnailKeyItr = m_thumbnailKeys.find(assetId);
            if (thumbnailKeyItr != m_thumbnailKeys.end())
            {
                const QVariant assetIdItemData(assetId.ToFixedString().c_str());
                const int index = findData(assetIdItemData);
                if (index >= 0)
                {
                    AzToolsFramework::Thumbnailer::SharedThumbnail thumbnail;
                    AzToolsFramework::Thumbnailer::ThumbnailerRequestBus::BroadcastResult(
                        thumbnail, &AzToolsFramework::Thumbnailer::ThumbnailerRequests::GetThumbnail, thumbnailKeyItr->second);
                    if (thumbnail)
                    {
                        // Adding pixmaps for all of the states so they don't disappear when highlighting 
                        const QPixmap pixmap = thumbnail->GetPixmap().scaled(iconSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        QIcon icon;
                        icon.addPixmap(pixmap, QIcon::Normal);
                        icon.addPixmap(pixmap, QIcon::Disabled);
                        icon.addPixmap(pixmap, QIcon::Active);
                        icon.addPixmap(pixmap, QIcon::Selected);
                        setItemIcon(index, icon);
                    }
                }
            }
        }
    }

    void AssetSelectionComboBox::QueueUpdateThumbnail(const AZ::Data::AssetId& assetId)
    {
        QTimer::singleShot(m_thumbnailDelayMs, this, [this, assetId]() { UpdateThumbnail(assetId); });
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/AssetSelection/moc_AssetSelectionComboBox.cpp>
