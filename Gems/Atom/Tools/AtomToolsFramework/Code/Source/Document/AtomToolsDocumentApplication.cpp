/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentApplication.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>

namespace AtomToolsFramework
{
    AtomToolsDocumentApplication::AtomToolsDocumentApplication(int* argc, char*** argv)
        : Base(argc, argv)
    {
    }

    void AtomToolsDocumentApplication::ProcessCommandLine(const AZ::CommandLine& commandLine)
    {
        // Process command line options for opening documents on startup
        size_t openDocumentCount = commandLine.GetNumMiscValues();
        for (size_t openDocumentIndex = 0; openDocumentIndex < openDocumentCount; ++openDocumentIndex)
        {
            const AZStd::string openDocumentPath = commandLine.GetMiscValue(openDocumentIndex);

            AZ_Printf(GetBuildTargetName().c_str(), "Opening document: %s", openDocumentPath.c_str());
            AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::OpenDocument, openDocumentPath);
        }

        Base::ProcessCommandLine(commandLine);
    }
} // namespace AtomToolsFramework
