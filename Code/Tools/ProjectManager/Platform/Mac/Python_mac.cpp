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
    extern bool InsertPythonLibraryPath(AZStd::unordered_set<AZStd::string>& paths, const char* pythonPackage, const char* engineRoot, const char* subPath);

    bool InsertPythonBinaryLibraryPaths(AZStd::unordered_set<AZStd::string>& paths, const char* pythonPackage, const char* engineRoot)
    {
        bool succeeded = true;
        
        // PY_VERSION_MAJOR_MINOR must be defined through the build scripts based on the current python package (see cmake/LYPython.cmake)
        #if !defined(PY_VERSION_MAJOR_MINOR)
        #error "PY_VERSION_MAJOR_MINOR is not defined"
        #endif

        succeeded = succeeded && InsertPythonLibraryPath(paths, pythonPackage, engineRoot, "python/runtime/%s/Python.framework/Versions/" PY_VERSION_MAJOR_MINOR "/lib");
        succeeded = succeeded && InsertPythonLibraryPath(paths, pythonPackage, engineRoot, "python/runtime/%s/Python.framework/Versions/" PY_VERSION_MAJOR_MINOR "/lib/python" PY_VERSION_MAJOR_MINOR "/lib-dynload");
        succeeded = succeeded && InsertPythonLibraryPath(paths, pythonPackage, engineRoot, "python/runtime/%s/Python.framework/Versions/" PY_VERSION_MAJOR_MINOR "/lib/python" PY_VERSION_MAJOR_MINOR);
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
