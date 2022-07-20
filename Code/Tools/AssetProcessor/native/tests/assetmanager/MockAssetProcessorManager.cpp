/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <tests/assetmanager/MockAssetProcessorManager.h>

namespace UnitTests
{
    void MockAssetProcessorManager::AssessAddedFile(QString filePath)
    {
        m_events[TestEvents::Added].Signal();
    }

    void MockAssetProcessorManager::AssessModifiedFile(QString filePath)
    {
        m_events[TestEvents::Modified].Signal();
    }

    void MockAssetProcessorManager::AssessDeletedFile(QString filePath)
    {
        m_events[TestEvents::Deleted].Signal();
    }
}
