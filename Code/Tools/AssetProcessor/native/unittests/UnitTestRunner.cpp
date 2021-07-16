/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UnitTestRunner.h"

#include <QElapsedTimer>
#include <QCoreApplication>


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

UnitTestRegistry* UnitTestRegistry::s_first = nullptr;

UnitTestRegistry::UnitTestRegistry(const char* name)
    : m_name(name)
{
    m_next = s_first;
    s_first = this;
}

int UnitTestRun::UnitTestPriority() const
{
    return 0;
}

const char* UnitTestRun::GetName() const
{
    return m_name;
}
void UnitTestRun::SetName(const char* name)
{
    m_name = name;
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

