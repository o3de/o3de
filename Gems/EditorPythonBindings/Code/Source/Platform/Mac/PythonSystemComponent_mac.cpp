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
        // PY_VERSION_MAJOR_MINOR must be defined through the build scripts based on the current python package (see cmake/LYPython.cmake)
        #if !defined(PY_VERSION_MAJOR_MINOR)
        #error "PY_VERSION_MAJOR_MINOR is not defined"
        #endif

        // append lib path to Python paths
        bool succeeded = true;
        
        succeeded = succeeded && InsertPythonLibraryPath(paths, pythonPackage, engineRoot, "python/runtime/%s/Python.framework/Versions/" PY_VERSION_MAJOR_MINOR "/lib");

        // append lib-dynload path
        succeeded = succeeded && InsertPythonLibraryPath(paths, pythonPackage, engineRoot, "python/runtime/%s/Python.framework/Versions/" PY_VERSION_MAJOR_MINOR "/lib/python" PY_VERSION_MAJOR_MINOR "/lib-dynload");

        // append base path to dynamic link libraries
        succeeded = succeeded && InsertPythonLibraryPath(paths, pythonPackage, engineRoot, "python/runtime/%s/Python.framework/Versions/" PY_VERSION_MAJOR_MINOR "/lib/python" PY_VERSION_MAJOR_MINOR);

        // append path to site-packages
        succeeded = succeeded && InsertPythonLibraryPath(paths, pythonPackage, engineRoot, "python/runtime/%s/Python.framework/Versions/" PY_VERSION_MAJOR_MINOR "/lib/python" PY_VERSION_MAJOR_MINOR "/site-packages");
        return succeeded;
    }

    AZStd::string GetPythonHomePath(const char* pythonPackage, const char* engineRoot)
    {
        // append lib path to Python paths
        AZ::IO::FixedMaxPath libPath = engineRoot;
        libPath /= AZ::IO::FixedMaxPathString::format("python/runtime/%s/Python.framework/Versions/" PY_VERSION_MAJOR_MINOR, pythonPackage);
        libPath = libPath.LexicallyNormal();
        return libPath.String();
    }
}
