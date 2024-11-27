/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
namespace AzToolsFramework::EmbeddedPython
{
    // When using embedded Python, some platforms need to explicitly load the python library.
    // For any modules that depend on 3rdParty::Python package, the AZ::Module should inherit this class.
    class PythonLoader
    {
    public:
        PythonLoader();
        ~PythonLoader();

        //! Calculate the python home (PYTHONHOME) based on the engine root
        //! @param engineRoot The path to the engine root to locate the python home
        //! @param overridePythonBaseVenvPath If set, the path override the base python venv folder
        //! @return The path of the python home path
        static AZ::IO::FixedMaxPath GetPythonHomePath(AZ::IO::PathView engineRoot, const char* overridePythonBaseVenvPath = nullptr);

        //! Collect the paths from all the egg-link files found in the python home
        //! paths used by the engine
        //! @param engineRoot The path to the engine root to locate the python home
        //! @param eggLinkPathVisitor The callback visitor function to receive the egg-link paths that are discovered
        //! @param overridePythonBaseVenvPath If set, the path override the base python venv folder
        using EggLinkPathVisitor = AZStd::function<void(AZ::IO::PathView)>;
        static void ReadPythonEggLinkPaths(AZ::IO::PathView engineRoot, EggLinkPathVisitor eggLinkPathVisitor, const char* overridePythonBaseVenvPath = nullptr);

        //! Calculate the path to the engine's python virtual environment used for
        //! python home (PYTHONHOME) based on the engine root
        //! @param engineRoot The path to the engine root to locate the python venv path
        //! @param overridePythonBaseVenvPath If set, the path override the base python venv folder
        //! @return The path of the python venv path
        static AZ::IO::FixedMaxPath GetPythonVenvPath(AZ::IO::PathView engineRoot, const char* overridePythonBaseVenvPath = nullptr);

        //! Calculate the path to the where the python executable resides in. Note that this
        //! is not always the same path as the python home path
        //! @param engineRoot The path to the engine root to
        //! @param overridePythonBaseVenvPath If set, the path override the base python venv folder
        //! locate the python executable path
        //! @return The path of the python venv path
        static AZ::IO::FixedMaxPath GetPythonExecutablePath(AZ::IO::PathView engineRoot, const char* overridePythonBaseVenvPath = nullptr);

    private:
        AZStd::unique_ptr<AZ::DynamicModuleHandle> m_embeddedLibPythonModuleHandle;
    };
} // namespace AzToolsFramework::EmbeddedPython
