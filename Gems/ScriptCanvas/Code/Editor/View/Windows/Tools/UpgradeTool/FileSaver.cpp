/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIOEventBus.h>
#include <Editor/View/Windows/Tools/UpgradeTool/FileSaver.h>

namespace FileSaverCpp
{
    class FileEventHandler
        : public AZ::IO::FileIOEventBus::Handler
    {
    public:
        int m_errorCode = 0;
        AZStd::string m_fileName;

        FileEventHandler()
        {
            BusConnect();
        }

        ~FileEventHandler()
        {
            BusDisconnect();
        }

        void OnError(const AZ::IO::SystemFile* /*file*/, const char* fileName, int errorCode) override
        {
            m_errorCode = errorCode;

            if (fileName)
            {
                m_fileName = fileName;
            }
        }
    };
}

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {

    }
}
