/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <AzToolsFramework/API/PythonLoader.h>
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
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Settings/ConfigParser.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/IO/LocalFileIO.h>

namespace AzToolsFramework::EmbeddedPython
{
    PythonLoader::PythonLoader()
    {
        #if defined(IMPLICIT_LOAD_PYTHON_SHARED_LIBRARY)

        // Determine if this is an sdk-engine build. For SDK engines, we want to prevent implicit python module loading.
        [[maybe_unused]] bool isSdkEngine{ false };
        auto engineSettingsPath = AZ::IO::FixedMaxPath{ AZ::Utils::GetEnginePath() } / "engine.json";
        if (AZ::IO::SystemFile::Exists(engineSettingsPath.c_str()))
        {
            auto loadOutcome = AZ::JsonSerializationUtils::ReadJsonFile(engineSettingsPath.c_str());
            if (loadOutcome.IsSuccess())
            {
                auto& doc = loadOutcome.GetValue();
                rapidjson::Value::MemberIterator sdkEngineFieldIter = doc.FindMember("sdk_engine");
                if (sdkEngineFieldIter != doc.MemberEnd())
                {
                    isSdkEngine = sdkEngineFieldIter->value.GetBool();
                }
            }
        }
        if (!isSdkEngine)
        {
            m_embeddedLibPythonModuleHandle = AZ::DynamicModuleHandle::Create(IMPLICIT_LOAD_PYTHON_SHARED_LIBRARY, false);
            [[maybe_unused]] bool loadResult = m_embeddedLibPythonModuleHandle->Load(AZ::DynamicModuleHandle::LoadFlags::GlobalSymbols);
            AZ_Error("PythonLoader", loadResult, "Failed to load " IMPLICIT_LOAD_PYTHON_SHARED_LIBRARY "\n");
        }
        #endif // IMPLICIT_LOAD_PYTHON_SHARED_LIBRARY
    }

    PythonLoader::~PythonLoader()
    {
        #if defined(IMPLICIT_LOAD_PYTHON_SHARED_LIBRARY)
        if (m_embeddedLibPythonModuleHandle)
        {
            m_embeddedLibPythonModuleHandle->Unload();
        }
        #endif // IMPLICIT_LOAD_PYTHON_SHARED_LIBRARY
    }

    AZ::IO::FixedMaxPath PythonLoader::GetPythonHomePath(AZ::IO::PathView engineRoot, const char* overridePythonBaseVenvPath /*= nullptr*/)
    {
        // The python HOME path relative to the executable depends on the host platform the package is created for
        #if AZ_TRAIT_PYTHON_LOADER_PYTHON_HOME_BIN_SUBPATH
        AZ::IO::FixedMaxPath pythonHomePath = PythonLoader::GetPythonExecutablePath(engineRoot, overridePythonBaseVenvPath).ParentPath();
        #else
        AZ::IO::FixedMaxPath pythonHomePath = PythonLoader::GetPythonExecutablePath(engineRoot, overridePythonBaseVenvPath);
        #endif // AZ_TRAIT_PYTHON_LOADER_PYTHON_HOME_BIN_SUBPATH

        return pythonHomePath;
    }

    AZ::IO::FixedMaxPath PythonLoader::GetPythonVenvPath(AZ::IO::PathView engineRoot, const char* overridePythonBaseVenvPath /*= nullptr*/)
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
        AZ::IO::FixedMaxPath libPath = (overridePythonBaseVenvPath != nullptr) ? AZ::IO::FixedMaxPath(overridePythonBaseVenvPath) : 
                                                                                 AZ::Utils::GetO3dePythonVenvRoot();

        // The ID is based on the first 32 bits of the digest, and formatted to at least 8-character wide hexadecimal representation
        libPath /= AZ::IO::FixedMaxPathString::format("%08x", digest[0]);
        libPath = libPath.LexicallyNormal();
        return libPath;
    }

    AZ::IO::FixedMaxPath PythonLoader::GetPythonExecutablePath(AZ::IO::PathView engineRoot, const char* overridePythonBaseVenvPath /*= nullptr*/)
    {
        AZ::IO::FixedMaxPath pythonVenvConfig = PythonLoader::GetPythonVenvPath(engineRoot, overridePythonBaseVenvPath) / "pyvenv.cfg";
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
        [[maybe_unused]] const auto parseOutcome = AZ::Settings::ParseConfigFile(systemFileStream, parserSettings);
        AZ_Error("python", parseOutcome, "Python venv file at %s missing home key. Make sure to run python/get_python.", pythonVenvConfig.c_str());

        return AZ::IO::FixedMaxPath(pythonHome);
    }

    void PythonLoader::ReadPythonEggLinkPaths(AZ::IO::PathView engineRoot, EggLinkPathVisitor resultPathCallback, const char* overridePythonBaseVenvPath /*= nullptr*/)
    {
        // Get the python venv path
        AZ::IO::FixedMaxPath pythonVenvSitePackages =
            AZ::IO::FixedMaxPath(PythonLoader::GetPythonVenvPath(engineRoot, overridePythonBaseVenvPath)) / O3DE_PYTHON_SITE_PACKAGE_SUBPATH;

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
