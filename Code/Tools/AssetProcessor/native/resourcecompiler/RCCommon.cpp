/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "RCCommon.h"
#include <QHash>
#include <AzCore/StringFunc/StringFunc.h>

namespace AssetProcessor
{
    QueueElementID::QueueElementID(SourceAssetReference sourceAssetReference, QString platform, QString jobDescriptor)
        : m_sourceAssetReference(AZStd::move(sourceAssetReference))
        , m_platform(platform)
        , m_jobDescriptor(jobDescriptor)
    {
    }

    SourceAssetReference QueueElementID::GetSourceAssetReference() const
    {
        return m_sourceAssetReference;
    }

    QString QueueElementID::GetPlatform() const
    {
        return m_platform;
    }

    QString QueueElementID::GetJobDescriptor() const
    {
        return m_jobDescriptor;
    }

    void QueueElementID::SetSourceAssetReference(SourceAssetReference sourceAssetReference)
    {
        m_sourceAssetReference = AZStd::move(sourceAssetReference);
    }

    void QueueElementID::SetPlatform(QString platform)
    {
        m_platform = platform;
    }

    void QueueElementID::SetJobDescriptor(QString jobDescriptor)
    {
        m_jobDescriptor = jobDescriptor;
    }

    bool QueueElementID::operator==(const QueueElementID& other) const
    {
        // if this becomes a hotspot in profile, we could use CRCs or other boost to comparison here.  These classes are constructed rarely
        // compared to how commonly they are compared with each other.
        return (
            m_sourceAssetReference == other.m_sourceAssetReference &&
            (QString::compare(m_platform, other.m_platform, Qt::CaseInsensitive) == 0) &&
            (QString::compare(m_jobDescriptor, other.m_jobDescriptor, Qt::CaseInsensitive) == 0)
            );
    }

    bool QueueElementID::operator<(const QueueElementID& other) const
    {
        int compare = m_sourceAssetReference.AbsolutePath().Compare(other.m_sourceAssetReference.AbsolutePath());
        if (compare != 0)
        {
            return (compare < 0);
        }

        compare = QString::compare(m_platform, other.m_platform, Qt::CaseInsensitive);
        if (compare != 0)
        {
            return (compare < 0);
        }

        compare = QString::compare(m_jobDescriptor, other.m_jobDescriptor, Qt::CaseInsensitive);

        if (compare != 0)
        {
            return (compare < 0);
        }

        // all three are equal, other is not less than this.
        return false;
    }

    uint qHash(const AssetProcessor::QueueElementID& key, uint seed)
    {
        return qHash(QString(key.GetSourceAssetReference().AbsolutePath().c_str()).toLower() + key.GetPlatform().toLower() + key.GetJobDescriptor().toLower(), seed);
    }
} // end namespace AssetProcessor

