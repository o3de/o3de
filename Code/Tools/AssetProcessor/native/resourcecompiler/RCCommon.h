/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef ASSETPROCESSOR_RCCOMMON_H
#define ASSETPROCESSOR_RCCOMMON_H

#include <QString>

namespace AssetProcessor
{
    // RCCommon contains common structs used by the RC Job System

    //! Job Exit codes.
    // note that this is NOT a complete list of return codes, and RC.EXE itself may return unknown return codes here.

    enum JobExitCodes : int
    {
        JobExitCode_Success = 0,
        JobExitCode_Failed = -10,
        JobExitCode_InvalidParams = -9,
        JobExitCode_UnableToCreateTempDir = -8,
        JobExitCode_CopyFailed = -6,
        JobExitCode_Unknown = -1,
        JobExitCode_RCNotFound = -4,
        JobExitCode_RCCouldNotBeLaunched = -5,
        JobExitCode_JobCancelled = -7,
    };


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

#endif //ASSETPROCESSOR_RCQUEUESORTMODEL_H
