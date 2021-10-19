/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QString>
#include <QDateTime>
#endif

namespace O3DE::ProjectManager
{
    class GemRepoInfo
    {
    public:
        GemRepoInfo() = default;
        GemRepoInfo(
            const QString& name,
            const QString& creator,
            const QDateTime& lastUpdated,
            bool isEnabled);

        bool IsValid() const;

        bool operator<(const GemRepoInfo& gemRepoInfo) const;

        QString m_path = "";
        QString m_name = "Unknown Repo Name";
        QString m_creator = "Unknown Creator";
        bool m_isEnabled = false; //! Is the repo currently enabled for this engine?
        QString m_summary = "No summary provided.";
        QString m_additionalInfo = "";
        QString m_directoryLink = "";
        QString m_repoUri = "";
        QStringList m_includedGemPaths = {};
        QDateTime m_lastUpdated;
    };
} // namespace O3DE::ProjectManager
