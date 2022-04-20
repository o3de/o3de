/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MockFileProcessor.h"

namespace UnitTests
{
     void MockFileProcessor::AssessAddedFile(QString fileName)
    {
        m_events[TestEvents::Added].Signal();
    }

     void MockFileProcessor::AssessDeletedFile(QString fileName)
    {
        m_events[TestEvents::Deleted].Signal();
    }
}
