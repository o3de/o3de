/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QObject>
#include <native/AssetManager/assetProcessorManager.h>
#include <tests/assetmanager/TestEventSignal.h>

namespace UnitTests
{
    struct MockAssetProcessorManager : ::AssetProcessor::AssetProcessorManager
    {
        Q_OBJECT

        using AssetProcessorManager::AssetProcessorManager;

    public Q_SLOTS:
        void AssessAddedFile(QString filePath) override;
        void AssessModifiedFile(QString filePath) override;
        void AssessDeletedFile(QString filePath) override;

    public:

        TestEventPair m_events[TestEvents::NumEvents];
    };
}
