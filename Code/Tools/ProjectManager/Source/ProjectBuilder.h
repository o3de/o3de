/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QFuture>
#endif

namespace O3DE::ProjectManager
{
    class ProjectBuilder
    {
    public:
        explicit ProjectBuilder();
        void AddJob(const QString& projectPath);

    private:
        QList<QFuture<QString>> m_futures;
        QList<QFutureWatcher<QFuture<QString>>> m_futureWatchers;
    };
} // namespace O3DE::ProjectManager
