/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef ASSETPROCESSOR_RCCOMMON_H
#define ASSETPROCESSOR_RCCOMMON_H

#include <AzCore/std/functional.h>
#include <QString>

namespace AssetProcessor
{
    //! Identifies a queued job uniquely.  Not a case sensitive compare
    class QueueElementID
    {
    public:
        QueueElementID() = default;
        
        ///! note that inputAssetName is a database name, not a relative path.
        QueueElementID(QString inputAssetName, QString platform, QString jobDescriptor);

        
        QString GetInputAssetName() const; ///< This is the database name, with output prefix.
        QString GetPlatform() const;
        QString GetJobDescriptor() const;
        void SetInputAssetName(QString inputAssetName);
        void SetPlatform(QString platform);
        void SetJobDescriptor(QString jobDescriptor);
        bool operator==(const QueueElementID& other) const;
        bool operator<(const QueueElementID& other) const;

    protected:
        QString m_inputAssetName;
        QString m_platform;
        QString m_jobDescriptor;
    };
    uint qHash(const AssetProcessor::QueueElementID& key, uint seed = 0);
} // namespace AssetProcessor

namespace AZStd {
    // Implement the hash for QueueElementID, so it can be used as the key of AZStd::unordered_map
    template <>
    struct hash<AssetProcessor::QueueElementID>
    {
        inline size_t operator()(const AssetProcessor::QueueElementID& key) const
        {
            return AssetProcessor::qHash(key);
        }
    };
} // namespace AZStd

#endif //ASSETPROCESSOR_RCQUEUESORTMODEL_H
