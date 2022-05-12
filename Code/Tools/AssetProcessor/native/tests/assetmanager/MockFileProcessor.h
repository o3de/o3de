/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QObject>
#include <FileProcessor/FileProcessor.h>
#include <tests/assetmanager/TestEventSignal.h>

namespace UnitTests
{
    struct MockFileProcessor : ::AssetProcessor::FileProcessor
    {
        Q_OBJECT

        using FileProcessor::FileProcessor;

    public Q_SLOTS:
        void AssessAddedFile(QString fileName) override;
        void AssessDeletedFile(QString fileName) override;

    public:
        TestEventPair m_events[TestEvents::NumEvents];
    };
}
