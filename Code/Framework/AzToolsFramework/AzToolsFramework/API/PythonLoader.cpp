/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <AzToolsFramework/API/PythonLoader.h>
#include <AzToolsFramework_Traits_Platform.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Math/Sha1.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/tokenize.h>
#include <AzCore/Settings/ConfigParser.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/IO/LocalFileIO.h>

namespace AzToolsFramework::EmbeddedPython
{
    PythonLoader::PythonLoader()
    {
        #if AZ_TRAIT_PYTHON_LOADER_ENABLE_EXPLICIT_LOADING
        // PYTHON_SHARED_LIBRARY_PATH must be defined in the build scripts and referencing the path to the python shared library
        #if !defined(PYTHON_SHARED_LIBRARY_PATH)
        #error "PYTHON_SHARED_LIBRARY_PATH is not defined"
        #endif

        // Construct the path to the shared python library within the venv folder
        AZ::IO::FixedMaxPath engineRoot = AZ::IO::FixedMaxPath(AZ::Utils::GetEnginePath());
        AZ::IO::FixedMaxPath thirdPartyRoot = PythonLoader::GetDefault3rdPartyPath(false);
        AZ::IO::FixedMaxPath pythonVenvPath = PythonLoader::GetPythonVenvPath(thirdPartyRoot, engineRoot);

        AZ::IO::PathView libPythonName = AZ::IO::PathView(PYTHON_SHARED_LIBRARY_PATH).Filename();
        AZ::IO::FixedMaxPath pythonVenvLibPath = pythonVenvPath / "lib" / libPythonName;

        if (AZ::IO::SystemFile::Exists(pythonVenvLibPath.StringAsPosix().c_str()))
        {
            m_embeddedLibPythonModuleHandle = AZ::DynamicModuleHandle::Create(pythonVenvLibPath.StringAsPosix().c_str(), false);
            bool loadResult = m_embeddedLibPythonModuleHandle->Load(false, true);
            AZ_Error("PythonLoader", loadResult, "Failed to load %s.\n", libPythonName.StringAsPosix().c_str());
        }
        #endif // AZ_TRAIT_PYTHON_LOADER_ENABLE_EXPLICIT_LOADING
    }

    PythonLoader::~PythonLoader()
    {
        #if AZ_TRAIT_PYTHON_LOADER_ENABLE_EXPLICIT_LOADING
        AZ_Assert(m_embeddedLibPythonModuleHandle, "DynamicModuleHandle for python was not created");
        m_embeddedLibPythonModuleHandle->Unload();
        #endif // AZ_TRAIT_PYTHON_LOADER_ENABLE_EXPLICIT_LOADING
    }

    AZ::IO::FixedMaxPath PythonLoader::GetDefault3rdPartyPath(bool createOnDemand)
    {
        AZ::IO::FixedMaxPath thirdPartyEnvPathPath;

        // The highest priority for the 3rd party path is the environment variable 'LY_3RDPARTY_PATH'
        static constexpr const char* env3rdPartyKey = "LY_3RDPARTY_PATH";
        char env3rdPartyPath[AZ::IO::MaxPathLength] = { '\0' };
        auto envOutcome = AZ::Utils::GetEnv(AZStd::span(env3rdPartyPath), env3rdPartyKey);
        if (envOutcome && (strlen(env3rdPartyPath) > 0))
        {
            // If so, then use the path that is set as the third party path
            thirdPartyEnvPathPath = AZ::IO::FixedMaxPath(env3rdPartyPath).LexicallyNormal();
        }
        // The next priority is to read the 3rd party directory from the manifest file
        else if (auto manifest3rdPartyResult = AZ::Utils::Get3rdPartyDirectory(); manifest3rdPartyResult.IsSuccess())
        {
            thirdPartyEnvPathPath = manifest3rdPartyResult.GetValue();
        }
        // Fallback to the default 3rd Party path based on the location of the manifest folder
        else
        {
            auto manifestPath = AZ::Utils::GetO3deManifestDirectory();
            thirdPartyEnvPathPath = AZ::IO::FixedMaxPath(manifestPath) / "3rdParty";
        }

        if ((!AZ::IO::SystemFile::IsDirectory(thirdPartyEnvPathPath.c_str())) && createOnDemand)
        {
            auto createPathResult = AZ::IO::SystemFile::CreateDir(thirdPartyEnvPathPath.c_str());
            AZ_Assert(createPathResult, "Unable to create missing 3rd Party Folder '%s'", thirdPartyEnvPathPath.c_str())
        }
        return thirdPartyEnvPathPath;
    }

    AZ::IO::FixedMaxPath PythonLoader::GetPythonHomePath(AZ::IO::PathView engineRoot)
    {
        AZ::IO::FixedMaxPath thirdPartyFolder = GetDefault3rdPartyPath(true);

        // The python HOME path relative to the executable depends on the host platform the package is created for
        #if AZ_TRAIT_PYTHON_LOADER_PYTHON_HOME_BIN_SUBPATH
        AZ::IO::FixedMaxPath pythonHomePath = PythonLoader::GetPythonExecutablePath(thirdPartyFolder, engineRoot).ParentPath();
        #else
        AZ::IO::FixedMaxPath pythonHomePath = PythonLoader::GetPythonExecutablePath(thirdPartyFolder, engineRoot);
        #endif // AZ_TRAIT_PYTHON_LOADER_PYTHON_HOME_BIN_SUBPATH

        return pythonHomePath;
    }

    AZ::IO::FixedMaxPath PythonLoader::GetPythonVenvPath(AZ::IO::PathView thirdPartyRoot, AZ::IO::PathView engineRoot)
    {
        // Perform the same hash calculation as cmake/CalculateEnginePathId.cmake
        /////

        // Prepare the engine path the same way as cmake/CalculateEnginePathId.cmake
        AZStd::string enginePath = AZ::IO::FixedMaxPath(engineRoot).StringAsPosix();
        enginePath += '/';
        AZStd::to_lower(enginePath.begin(), enginePath.end());

        // Perform a SHA1 hash on the prepared engine path
        AZ::Sha1 hasher;
        AZ::u32 digest[5];
        hasher.ProcessBytes(AZStd::as_bytes(AZStd::span(enginePath)));
        hasher.GetDigest(digest);

        // Construct the path to where the python venv based on the engine path should be located
        AZ::IO::FixedMaxPath libPath = thirdPartyRoot;
        // The ID is based on the first 32 bits of the digest, and formatted to at least 8-character wide hexadecimal representation
        libPath /= AZ::IO::FixedMaxPathString::format("venv/%08x", digest[0]);
        libPath = libPath.LexicallyNormal();
        return libPath;
    }

    AZ::IO::FixedMaxPath PythonLoader::GetPythonExecutablePath(AZ::IO::PathView thirdPartyRoot, AZ::IO::PathView engineRoot)
    {
        AZ::IO::FixedMaxPath pythonVenvConfig = PythonLoader::GetPythonVenvPath(thirdPartyRoot, engineRoot) / "pyvenv.cfg";
        AZ::IO::SystemFileStream systemFileStream;
        if (!systemFileStream.Open(pythonVenvConfig.c_str(), AZ::IO::OpenMode::ModeRead))
        {
            AZ_Error("python", false, "Missing python venv file at %s. Make sure to run python/get_python.", pythonVenvConfig.c_str());
            return "";
        }

        AZ::Settings::ConfigParserSettings parserSettings;
        AZ::IO::FixedMaxPathString pythonHome;
        parserSettings.m_parseConfigEntryFunc = [&pythonHome](const AZ::Settings::ConfigParserSettings::ConfigEntry& configEntry)
        {
            if (AZ::StringFunc::Equal(configEntry.m_keyValuePair.m_key, "home"))
            {
                pythonHome = configEntry.m_keyValuePair.m_value;
            }
            return true;
        };
        const auto parseOutcome = AZ::Settings::ParseConfigFile(systemFileStream, parserSettings);
        AZ_Error("python", parseOutcome, "Python venv file at %s missing home key. Make sure to run python/get_python.", pythonVenvConfig.c_str());

        return AZ::IO::FixedMaxPath(pythonHome);
    }

    void PythonLoader::ReadPythonEggLinkPaths(AZ::IO::PathView thirdPartyRoot, AZ::IO::PathView engineRoot, EggLinkPathVisitor resultPathCallback)
    {
        // Get the python venv path
        AZ::IO::FixedMaxPath pythonVenvSitePackages =
            AZ::IO::FixedMaxPath(PythonLoader::GetPythonVenvPath(thirdPartyRoot, engineRoot)) / O3DE_PYTHON_SITE_PACKAGE_SUBPATH;

        // Always add the site-packages folder from the virtual environment into the path list
        resultPathCallback(pythonVenvSitePackages.LexicallyNormal());

        // pybind11 does not resolve any .egg-link files, so any packages that there pip-installed into the venv as egg-links
        // are not getting resolved. We will do this manually by opening the egg-links in the venv site-packages path and injecting
        // the non local paths as well
        AZ::IO::LocalFileIO localFileSystem;
        localFileSystem.FindFiles(
            pythonVenvSitePackages.c_str(),
            "*.egg-link",
            [&resultPathCallback](const char* filePath) -> bool
            {
                auto readFileResult = AZ::Utils::ReadFile(filePath);
                if (readFileResult)
                {
                    auto eggLinkContent = readFileResult.GetValue();

                    AZStd::vector<AZStd::string> eggLinkLines;
                    AZStd::string delim = "\r\n";
                    AZStd::tokenize(eggLinkContent, delim, eggLinkLines);
                    for (auto eggLinkLine : eggLinkLines)
                    {
                        if (eggLinkLine.compare(".") != 0)
                        {
                            resultPathCallback(AZ::IO::PathView(eggLinkLine));
                        }
                    }
                }
                return true;
            }
        );
    }
}
