/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <native/unittests/UnitTestUtils.h>

#include <QElapsedTimer>
#include <QTextStream>
#include <QCoreApplication>
#include <AzCore/IO/Path/Path.h>


namespace AssetProcessorBuildTarget
{
    //! This function returns the build system target name
    AZStd::string_view GetBuildTargetName()
    {
#if !defined (LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return AZStd::string_view{ LY_CMAKE_TARGET };
    }
}

namespace UnitTestUtils
{
    void SleepForMinimumFileSystemTime()
    {
        // note that on Mac, the file system has a resolution of 1 second, and since we're using modtime for a bunch of things,
        // not the actual hash files, we have to wait different amount depending on the OS.
#ifdef AZ_PLATFORM_WINDOWS
        int milliseconds = 1;
#else
        int milliseconds = 1001;
#endif
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(milliseconds));
    }

    bool CreateDummyFileAZ(AZ::IO::PathView fullPathToFile, AZStd::string_view contents)
    {
        AZ::IO::FileIOStream stream(fullPathToFile.FixedMaxPathString().c_str(), AZ::IO::OpenMode::ModeWrite|AZ::IO::OpenMode::ModeCreatePath);

        if (!stream.IsOpen())
        {
            return false;
        }

        stream.Write(contents.size(), contents.data());
        stream.Close();

        return true;
    }

    bool CreateDummyFile(const QString& fullPathToFile, QString contents)
    {
        QFileInfo fi(fullPathToFile);
        QDir fp(fi.path());
        fp.mkpath(".");
        QFile writer(fullPathToFile);
        if (!writer.open(QFile::WriteOnly))
        {
            return false;
        }

        if (!contents.isEmpty())
        {
            QTextStream ts(&writer);
            ts.setCodec("UTF-8");
            ts << contents;
        }
        return true;
    }

    bool BlockUntil(bool& varToWatch, int millisecondsMax)
    {
        QElapsedTimer limit;
        limit.start();
        varToWatch = false;
        while ((!varToWatch) && (limit.elapsed() < millisecondsMax))
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        }

        // and then once more, so that any queued events as a result of the above finish.
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);

        return (varToWatch);
    }

}

