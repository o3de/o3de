/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Path/Path.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace Platform
{
    bool InsertPythonLibraryPath(AZStd::unordered_set<AZStd::string>& paths, const char* pythonPackage, const char* engineRoot, const char* subPath)
    {
        // append lib path to Python paths
        AZ::IO::FixedMaxPath libPath = engineRoot;
        libPath /= AZ::IO::FixedMaxPathString::format(subPath, pythonPackage);
        libPath = libPath.LexicallyNormal();
        if (AZ::IO::SystemFile::Exists(libPath.c_str()))
        {
            paths.insert(libPath.c_str());
            return true;
        }
        
        AZ_Warning("python", false, "Python library path should exist! path:%s", libPath.c_str());
        return false;
    }

    bool InsertPythonBinaryLibraryPaths(AZStd::unordered_set<AZStd::string>& paths, const char* pythonPackage, const char* engineRoot)
    {
        // append lib path to Python paths
        bool succeeded = true;
        
        succeeded = succeeded && InsertPythonLibraryPath(paths, pythonPackage, engineRoot, "python/runtime/%s/python");
        succeeded = succeeded && InsertPythonLibraryPath(paths, pythonPackage, engineRoot, "python/runtime/%s/python/lib");
        succeeded = succeeded && InsertPythonLibraryPath(paths, pythonPackage, engineRoot, "python/runtime/%s/python/lib/site-packages");
        succeeded = succeeded && InsertPythonLibraryPath(paths, pythonPackage, engineRoot, "python/runtime/%s/python/DLLs");
        return succeeded;
    }

    AZStd::string GetPythonHomePath(const char* pythonPackage, const char* engineRoot)
    {
        // append lib path to Python paths
        AZ::IO::FixedMaxPath libPath = engineRoot;
        libPath /= AZ::IO::FixedMaxPathString::format("python/runtime/%s/python", pythonPackage);
        libPath = libPath.LexicallyNormal();
        return libPath.String();
    }
}
