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
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/SourceThumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

#include <QAbstractItemView>
#include <QFileInfo>
#include <QTimer>

namespace AtomToolsFramework
{
    AssetSelectionComboBox::AssetSelectionComboBox(const FilterFn& filterFn, QWidget* parent)
        : QComboBox(parent)
        , m_filterFn(filterFn)
    {
        QSignalBlocker signalBlocker(this);

        setDuplicatesEnabled(true);
        setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy::AdjustToContents);
        view()->setMinimumWidth(200);

        connect(
            this, static_cast<void (QComboBox::*)(const int)>(&QComboBox::currentIndexChanged), this,
            [this]() { emit PathSelected(GetSelectedPath()); });

        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
    }

    AssetSelectionComboBox::~AssetSelectionComboBox()
    {
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
    }

    void AssetSelectionComboBox::Clear()
    {
        clear();
        m_thumbnailKeys.clear();
    }

    void AssetSelectionComboBox::Populate()
    {
        Clear();

        for (const auto& path : GetPathsInSourceFoldersMatchingFilter(m_filterFn))
        {
            AddPath(path);
        }

        setCurrentIndex(0);
    }

    void AssetSelectionComboBox::SetFilter(const FilterFn& filterFn)
    {
        m_filterFn = filterFn;
    }

    const AssetSelectionComboBox::FilterFn& AssetSelectionComboBox::GetFilter() const
    {
        return m_filterFn;
    }

    void AssetSelectionComboBox::AddPath(const AZStd::string& path)
    {
        if (m_filterFn && !m_filterFn(path))
        {
            return;
        }

        const QVariant pathItemData(QString::fromUtf8(path.data(), static_cast<int>(path.size())));
        if (const int index = findData(pathItemData); index < 0)
        {
            addItem(GetDisplayNameFromPath(path).c_str(), pathItemData);
            QueueSort();
            RegisterThumbnail(path);
        }
    }

    void AssetSelectionComboBox::RemovePath(const AZStd::string& path)
    {
        const QVariant pathItemData(QString::fromUtf8(path.data(), static_cast<int>(path.size())));
        if (const int index = findData(pathItemData); index >= 0)
        {
            removeItem(index);
            m_thumbnailKeys.erase(path);
        }
    }

    void AssetSelectionComboBox::SelectPath(const AZStd::string& path)
    {
        const QVariant pathItemData(QString::fromUtf8(path.data(), static_cast<int>(path.size())));
        if (const int index = findData(pathItemData); index >= 0)
        {
            setCurrentIndex(index);
        }
    }

    AZStd::string AssetSelectionComboBox::GetSelectedPath() const
    {
        return currentData().toString().toUtf8().constData();
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
                RegisterThumbnail(itemData(index).toString().toUtf8().constData());
            }
        }
    }

    void AssetSelectionComboBox::SetThumbnailDelayMs(AZ::u32 delay)
    {
        m_thumbnailDelayMs = delay;
    }

    void AssetSelectionComboBox::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        AddPath(AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetId));
    }

    void AssetSelectionComboBox::OnCatalogAssetRemoved(
        const AZ::Data::AssetId& assetId, [[maybe_unused]] const AZ::Data::AssetInfo& assetInfo)
    {
        RemovePath(AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetId));
    }

    void AssetSelectionComboBox::RegisterThumbnail(const AZStd::string& path)
    {
        if (m_thumbnailsEnabled)
        {
            bool result = false;
            AZ::Data::AssetInfo assetInfo;
            AZStd::string watchFolder;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                result,
                &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath,
                path.data(),
                assetInfo,
                watchFolder);

            AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey =
                MAKE_TKEY(AzToolsFramework::AssetBrowser::SourceThumbnailKey, assetInfo.m_assetId.m_guid);
            m_thumbnailKeys[path] = thumbnailKey;

            connect(
                thumbnailKey.data(), &AzToolsFramework::Thumbnailer::ThumbnailKey::ThumbnailUpdatedSignal, this,
                [this, path]() { QueueUpdateThumbnail(path); });

            QueueUpdateThumbnail(path);
        }
    }

    void AssetSelectionComboBox::UpdateThumbnail(const AZStd::string& path)
    {
        if (m_thumbnailsEnabled)
        {
            auto thumbnailKeyItr = m_thumbnailKeys.find(path);
            if (thumbnailKeyItr != m_thumbnailKeys.end())
            {
                const QVariant pathItemData(QString::fromUtf8(path.data(), static_cast<int>(path.size())));
                if (const int index = findData(pathItemData); index >= 0)
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

    void AssetSelectionComboBox::QueueUpdateThumbnail(const AZStd::string& path)
    {
        QTimer::singleShot(m_thumbnailDelayMs, this, [this, path]() {
            UpdateThumbnail(path);
        });
    }

    void AssetSelectionComboBox::QueueSort()
    {
        if (!m_queueSort)
        {
            m_queueSort = true;
            QTimer::singleShot(0, this, [this]() {
                m_queueSort = false;
                model()->sort(0, Qt::AscendingOrder);
            });
        }
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/AssetSelection/moc_AssetSelectionComboBox.cpp>
