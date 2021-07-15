/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectBuilderWorker.h>

namespace O3DE::ProjectManager
{
    AZ::Outcome<void, QString> ProjectBuilderWorker::BuildProjectForPlatform()
    {
        QString error = tr("Automatic building on Linux not currently supported!");
        QStringToAZTracePrint(error);
        return AZ::Failure(error);
    }
    
} // namespace O3DE::ProjectManager
