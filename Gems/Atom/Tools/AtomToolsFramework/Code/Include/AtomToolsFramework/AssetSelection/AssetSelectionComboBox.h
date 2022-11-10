/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <QComboBox>
#endif

namespace AtomToolsFramework
{
    class AssetSelectionComboBox : public QComboBox
    {
        Q_OBJECT
    public:
        AssetSelectionComboBox(const AZStd::function<bool(const AZStd::string&)>& filterCallback, QWidget* parent = 0);
        ~AssetSelectionComboBox();

        void Reset();
        void SetFilter(const AZStd::function<bool(const AZStd::string&)>& filterCallback);
        void SelectPath(const AZStd::string& path);
        AZStd::string GetSelectedPath() const;

        void SetThumbnailsEnabled(bool enabled);
        void SetThumbnailDelayMs(AZ::u32 delay);

    Q_SIGNALS:
        void PathSelected(const AZStd::string& path);

    private:
        AZ_DISABLE_COPY_MOVE(AssetSelectionComboBox);

        void AddPath(const AZStd::string& path);
        void RegisterThumbnail(const AZStd::string& path);
        void UpdateThumbnail(const AZStd::string& path);
        void QueueUpdateThumbnail(const AZStd::string& path);
        void QueueSort();

        AZStd::function<bool(const AZStd::string&)> m_filterCallback;

        bool m_thumbnailsEnabled = false;
        AZ::u32 m_thumbnailDelayMs = 2000;
        AZStd::unordered_map<AZStd::string, AzToolsFramework::Thumbnailer::SharedThumbnailKey> m_thumbnailKeys;
        bool m_queueSort = false;
    };
} // namespace AtomToolsFramework
