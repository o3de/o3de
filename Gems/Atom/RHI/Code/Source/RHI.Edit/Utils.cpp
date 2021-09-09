/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Edit/Utils.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

#include <AzFramework/Process/ProcessCommunicator.h>
#include <AzFramework/Process/ProcessWatcher.h>
#include <AzToolsFramework/Debug/TraceContext.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/Platform.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <md5.h>

namespace AZ
{
    namespace RHI
    {
        static AZStd::mutex s_profilingMutex;
        [[maybe_unused]] static constexpr char ShaderPlatformInterfaceName[] = "ShaderPlatformInterface";

        void ShaderCompilerProfiling::Entry::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<Entry>()
                    ->Version(0)
                    ->Field("ExecutablePath", &Entry::m_executablePath)
                    ->Field("Parameters", &Entry::m_parameters)
                    ->Field("ElapsedTimeSeconds", &Entry::m_elapsedTimeSeconds)
                    ;
            }
        }

        void ShaderCompilerProfiling::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderCompilerProfiling>()
                    ->Version(0)
                    ->Field("Entries", &ShaderCompilerProfiling::m_entries)
                    ;
            }

            Entry::Reflect(context);
        }

        //! Helper function to write a new entry in the shader compilation profiling data (see the ShaderCompilerProfiling struct).
        //! The function tries to read the existing JSON file if it exists, and appends a new entry for a compilation request.
        //! The profiling data consists of the compiler executable, the parameters used to call it and the elapsed time for that call.
        bool WriteProfilingEntryToLog(const AZStd::string& shaderPath, const ShaderCompilerProfiling::Entry& profilingEntry)
        {
            AZStd::lock_guard<AZStd::mutex> lock(s_profilingMutex);

            AZStd::string folderPath;
            AzFramework::StringFunc::Path::GetFullPath(shaderPath.data(), folderPath);

            AZStd::string fileName;
            AzFramework::StringFunc::Path::GetFileName(shaderPath.data(), fileName);
            AzFramework::StringFunc::Path::ReplaceExtension(fileName, "");

            const AZ::Platform::ProcessId processId = AZ::Platform::GetCurrentProcessId();

            AZStd::string filePath = shaderPath;
            AZStd::string logName = AZStd::string::format("%s.%d.profiling", fileName.data(), processId);
            AzFramework::StringFunc::Path::Join(folderPath.data(), logName.data(), filePath);

            RHI::ShaderCompilerProfiling profiling;

            if (IO::FileIOBase::GetInstance()->Exists(filePath.data()))
            {
                auto loadResult = AZ::JsonSerializationUtils::LoadObjectFromFile<RHI::ShaderCompilerProfiling>(profiling, filePath);

                if (!loadResult.IsSuccess())
                {
                    AZ_Error(ShaderPlatformInterfaceName, false, "Failed to load shader compiler profiling from file [%s]", filePath.data());
                    AZ_Error(ShaderPlatformInterfaceName, false, "Loading issues: %s", loadResult.GetError().data());
                    return false;
                }
            }

            profiling.m_entries.push_back(profilingEntry);

            auto saveResult = AZ::JsonSerializationUtils::SaveObjectToFile<RHI::ShaderCompilerProfiling>(&profiling, filePath);
            if (!(saveResult.IsSuccess()))
            {
                AZ_Error(ShaderPlatformInterfaceName, false, "Failed to save shader compiler profiling to file %s", filePath.data());
                AZ_Error(ShaderPlatformInterfaceName, false, "Saving issues: %s", saveResult.GetError().data());
                return false;
            }

            return true;
        }

        AZStd::string PrependFile(PrependArguments& arguments)
        {
            static const char* executableFolder = nullptr;
            const auto AsAbsolute = [](const AZStd::string& localFile) -> AZStd::optional<AZStd::string> 
            {
                if (!AzFramework::StringFunc::Path::IsRelative(localFile.c_str()))
                {
                    return localFile;
                }

                if (!executableFolder)
                {
                    AZ::ComponentApplicationBus::BroadcastResult(executableFolder, &AZ::ComponentApplicationBus::Events::GetExecutableFolder);
                    if (!executableFolder)
                    {
                        AZ_Error(ShaderPlatformInterfaceName, false, "Unable to determine application root.");
                        return AZStd::nullopt;
                    }
                }

                AZStd::string fileAbsolutePath;
                AzFramework::StringFunc::Path::Join(executableFolder, localFile.c_str(), fileAbsolutePath);
                return fileAbsolutePath;
            };

            // If either file path is empty there is nothing to prepend. Return the original source file path (even if empty).
            if (!arguments.m_prependFile || !arguments.m_sourceFile)
            {
                return arguments.m_sourceFile;
            }

            // Read the prepended file
            auto prependFileAbsolutePath = AsAbsolute(arguments.m_prependFile);
            if (!prependFileAbsolutePath)
            {
                return arguments.m_sourceFile;
            }

            if (!AZ::IO::SystemFile::Exists(prependFileAbsolutePath->c_str()))
            {
                AZ_Warning(ShaderPlatformInterfaceName, false, "Missing prepend file: '%s'. Will continue without prepending.", prependFileAbsolutePath->c_str());
                return arguments.m_sourceFile;
            }

            auto prependFileLoadResult = LoadFileString(prependFileAbsolutePath->c_str());
            if (!prependFileLoadResult)
            {
                AZ_Error(ShaderPlatformInterfaceName, false, "%s", prependFileLoadResult.GetError().c_str());
                return arguments.m_sourceFile;
            }
            
            auto sourceFileAbsolutePath = AsAbsolute(arguments.m_sourceFile);
            if (!sourceFileAbsolutePath)
            {
                return arguments.m_sourceFile;
            }

            auto sourceFileLoadResult = LoadFileString(sourceFileAbsolutePath->c_str());
            if (!sourceFileLoadResult)
            {
                AZ_Error(ShaderPlatformInterfaceName, false, "%s", sourceFileLoadResult.GetError().c_str());
                return *sourceFileAbsolutePath;
            }

            AZStd::string combinedFile;
            if (arguments.m_destinationFolder)
            {
                AZStd::string filename;
                if(AzFramework::StringFunc::Path::GetFullFileName(sourceFileAbsolutePath->c_str(), filename))
                {
                    combinedFile = AZStd::string::format("%s/%s", arguments.m_destinationFolder, filename.c_str());
                }
                else
                {
                    AZ_Error(ShaderPlatformInterfaceName, false, "GetFullFileName('%s') failed", sourceFileAbsolutePath->c_str());
                    return *sourceFileAbsolutePath;
                }
            }
            else
            {
                combinedFile = *sourceFileAbsolutePath;
            }
            combinedFile += (arguments.m_addSuffixToFileName ? "." + AZStd::string{ arguments.m_addSuffixToFileName } : "") + ".prepend";

            if (arguments.m_destinationStringOpt)
            {
                *arguments.m_destinationStringOpt = prependFileLoadResult.GetValue().c_str();
                *arguments.m_destinationStringOpt += sourceFileLoadResult.GetValue().c_str();
            }
            else
            {
                // Write the file back to disk (if requested) so the native shader compiler can read it.
                AZ::IO::FileIOStream combinedFileStream(combinedFile.c_str(), AZ::IO::OpenMode::ModeWrite);
                if (!combinedFileStream.IsOpen())
                {
                    AZ_Error(ShaderPlatformInterfaceName, false, "Failed to open output file %s", combinedFile.c_str());
                    return *sourceFileAbsolutePath;
                }

                combinedFileStream.Write(prependFileLoadResult.GetValue().size(), prependFileLoadResult.GetValue().data());
                combinedFileStream.Write(sourceFileLoadResult.GetValue().size(), sourceFileLoadResult.GetValue().data());
                combinedFileStream.Close();
            }

            if (arguments.m_digest)  // if the function's caller requested to compute a digest, let's hash the content and store it in the digest array.
            {            // this is useful for lld/pdb file naming (shader debug symbols) because it's the automatically recognized scheme by PIX and alike.
                MD5Context md5;
                MD5Init(&md5);
                MD5Update(&md5, reinterpret_cast<unsigned char*>(prependFileLoadResult.GetValue().data()), aznumeric_cast<unsigned>(prependFileLoadResult.GetValue().size()));
                MD5Update(&md5, reinterpret_cast<unsigned char*>(sourceFileLoadResult.GetValue().data()), aznumeric_cast<unsigned>(sourceFileLoadResult.GetValue().size()));
                MD5Final(*arguments.m_digest, &md5);
            }

            return combinedFile;
        }

        bool ExecuteShaderCompiler(const AZStd::string& executablePath,
                                   const AZStd::string& parameters,
                                   const AZStd::string& shaderSourcePathForDebug,
                                   const char* toolNameForLog)
        {
            AZStd::string executableAbsolutePath;
            if (AzFramework::StringFunc::Path::IsRelative(executablePath.c_str()))
            {
                static const char* executableFolder = nullptr;
                if (!executableFolder)
                {
                    AZ::ComponentApplicationBus::BroadcastResult(executableFolder, &AZ::ComponentApplicationBus::Events::GetExecutableFolder);
                    if (!executableFolder)
                    {
                        AZ_Error(ShaderPlatformInterfaceName, false, "Unable to determine application root.");
                        return false;
                    }
                }

                AzFramework::StringFunc::Path::Join(executableFolder, executablePath.c_str(), executableAbsolutePath);
            }
            else
            {
                executableAbsolutePath = executablePath;
            }

            if (!AZ::IO::SystemFile::Exists(executableAbsolutePath.c_str()))
            {
                AZ_Error(ShaderPlatformInterfaceName, false, "Executable not found: '%s'", executableAbsolutePath.c_str());
                return false;
            }

            AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
            processLaunchInfo.m_commandlineParameters = AZStd::string::format("\"%s\" %s", executableAbsolutePath.c_str(), parameters.c_str());
            processLaunchInfo.m_showWindow = true;
            processLaunchInfo.m_processPriority = AzFramework::ProcessPriority::PROCESSPRIORITY_NORMAL;

            {
                AZStd::string contextKey = toolNameForLog + AZStd::string(" Input File");
                AZ_TraceContext(contextKey, shaderSourcePathForDebug);
            }
            {
                AZStd::string contextKey = toolNameForLog + AZStd::string(" Command Line");
                AZ_TraceContext(contextKey, processLaunchInfo.m_commandlineParameters);
            }
            AZ_TracePrintf(ShaderPlatformInterfaceName, "Executing '%s' ...", processLaunchInfo.m_commandlineParameters.c_str());

            AzFramework::ProcessWatcher* watcher = AzFramework::ProcessWatcher::LaunchProcess(processLaunchInfo, AzFramework::COMMUNICATOR_TYPE_STDINOUT);
            if (!watcher)
            {
                AZ_Error(ShaderPlatformInterfaceName, false, "Shader compiler could not be launched");
                return false;
            }

            AZStd::unique_ptr<AzFramework::ProcessWatcher> watcherPtr = AZStd::unique_ptr<AzFramework::ProcessWatcher>(watcher);

            AZStd::string errorMessages;
            auto pumpOuputStreams = [&watcherPtr, &errorMessages]()
            {
                auto communicator = watcherPtr->GetCommunicator();
                
                // Instead of collecting all the output in a giant string, it would be better to report 
                // the chunks of messages as they arrive, but this should be good enough for now.
                if (auto byteCount = communicator->PeekError())
                {
                    AZStd::string chunk;
                    chunk.resize(byteCount);
                    communicator->ReadError(chunk.data(), byteCount);
                    errorMessages += chunk;
                }

                // Even though we don't use the stdout stream, we have to read it or the process will hang
                if (auto byteCount = communicator->PeekOutput())
                {
                    AZStd::string chunk;
                    chunk.resize(byteCount);
                    communicator->ReadOutput(chunk.data(), byteCount);
                }
            };

            uint32_t exitCode = 0;
            bool timedOut = false;

            const AZStd::sys_time_t maxWaitTimeSeconds = 300;
            const AZStd::sys_time_t startTimeSeconds = AZStd::GetTimeNowSecond();
            const AZStd::sys_time_t startTime = AZStd::GetTimeNowTicks();

            while (watcherPtr->IsProcessRunning(&exitCode))
            {
                const AZStd::sys_time_t currentTimeSeconds = AZStd::GetTimeNowSecond();
                if (currentTimeSeconds - startTimeSeconds > maxWaitTimeSeconds)
                {
                    timedOut = true;
                    static const uint32_t TimeOutExitCode = 121;
                    exitCode = TimeOutExitCode;
                    watcherPtr->TerminateProcess(TimeOutExitCode);
                    break;
                }
                else
                {
                    pumpOuputStreams();
                }
            }

            AZ_Assert(!watcherPtr->IsProcessRunning(), "Shader compiler execution failed to terminate");

            // Pump one last time to make sure the streams have been flushed
            pumpOuputStreams();

            const bool reportedErrors = ReportErrorMessages(toolNameForLog, errorMessages);

            if (timedOut)
            {
                AZ_Error(ShaderPlatformInterfaceName, false, "%s execution timed out after %d seconds", toolNameForLog, maxWaitTimeSeconds);
                return false;
            }

            if (exitCode != 0)
            {
                AZ_Error(ShaderPlatformInterfaceName, false, "%s has exited with error code %d", toolNameForLog, exitCode);
                return false;
            }

            if (reportedErrors)
            {
                AZ_Error(ShaderPlatformInterfaceName, false, "%s returned successfully, but errors were detected.", toolNameForLog);
                return false;
            }

            // Write the shader compiler profiling data in distinct JSON files.
            // We cannot use a shared data structure because distinct builder processes are running in parallel.
            const AZStd::sys_time_t endTime = AZStd::GetTimeNowTicks();
            const AZStd::sys_time_t deltaTime = endTime - startTime;
            const float elapsedTimeSeconds = (float)(deltaTime) / (float)AZStd::GetTimeTicksPerSecond();

            AZ_TracePrintf(ShaderPlatformInterfaceName, "%s elapsedTimeMillis: %d", __FUNCTION__, aznumeric_cast<int>(elapsedTimeSeconds * 1000.0f));

            ShaderCompilerProfiling::Entry profilingEntry;
            profilingEntry.m_executablePath = executablePath;
            profilingEntry.m_parameters = parameters;
            profilingEntry.m_elapsedTimeSeconds = elapsedTimeSeconds;
            WriteProfilingEntryToLog(shaderSourcePathForDebug, profilingEntry);

            return true;
        }

        bool ReportErrorMessages([[maybe_unused]] AZStd::string_view window, AZStd::string_view errorMessages)
        {
            // There are more efficient ways to do this, but this approach is simple and gets us moving for now.
            AZStd::vector<AZStd::string> lines;
            AzFramework::StringFunc::Tokenize(errorMessages.data(), lines, "\n\r");

            bool foundErrors = false;
            
            for (auto& line : lines)
            {
                if (AZStd::string::npos != AzFramework::StringFunc::Find(line, "error"))
                {
                    AZ_Error(window.data(), false, "%s", line.data());
                    foundErrors = true;
                }
                else if (AZStd::string::npos != AzFramework::StringFunc::Find(line, "warning"))
                {
                    AZ_Warning(window.data(), false, "%s", line.data());
                }
                else 
                {
                    AZ_TracePrintf(window.data(), "%s", line.data());
                }
            }

            return foundErrors;
        }

        ShaderStage ToRHIShaderStage(ShaderHardwareStage stageType)
        {
            switch (stageType)
            {
            case ShaderHardwareStage::Compute:
                return RHI::ShaderStage::Compute;
            case ShaderHardwareStage::Fragment:
                return RHI::ShaderStage::Fragment;
            case ShaderHardwareStage::Geometry:
                AZ_Assert(false, "RHI currently does not support geometry shaders");
                return RHI::ShaderStage::Unknown;
            case ShaderHardwareStage::TessellationControl:
            case ShaderHardwareStage::TessellationEvaluation:
                return RHI::ShaderStage::Tessellation;
            case ShaderHardwareStage::Vertex:
                return RHI::ShaderStage::Vertex;
            case ShaderHardwareStage::RayTracing:
                return RHI::ShaderStage::RayTracing;
            }
            AZ_Assert(false, "Unable to find RHI Shader stage given RPI ShaderStageType %d", stageType);
            return RHI::ShaderStage::Unknown;
        }

        Outcome<AZStd::string, AZStd::string> LoadFileString(const char* path)
        {
            AZ::IO::FileIOStream fileStream(path, AZ::IO::OpenMode::ModeRead);
            if (!fileStream.IsOpen())
            {
                return AZ::Failure(AZStd::string::format("Could not open file '%s'.", path));
            }

            AZStd::string text;
            text.resize(fileStream.GetLength());

            auto bytesRead = fileStream.Read(text.size(), text.data());
            if (bytesRead != fileStream.GetLength())
            {
                return AZ::Failure(AZStd::string::format("Failed to load file '%s'.", path));
            }
            
            return AZ::Success(AZStd::move(text));
        }

        Outcome<AZStd::vector<uint8_t>, AZStd::string> LoadFileBytes(const char* path)
        {
            AZ::IO::FileIOStream fileStream(path, AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary);
            if (!fileStream.IsOpen())
            {
                return AZ::Failure(AZStd::string::format("Could not open file '%s'.", path));
            }

            AZStd::vector<uint8_t> bytes;
            bytes.resize(fileStream.GetLength());

            auto bytesRead = fileStream.Read(bytes.size(), bytes.data());
            if (bytesRead != fileStream.GetLength())
            {
                return AZ::Failure(AZStd::string::format("Failed to load file '%s'.", path));
            }
            
            return AZ::Success(AZStd::move(bytes));
        }

        size_t RegexCount(AZStd::string_view text, const char* regex)
        {
            size_t count = 0;
            AZStd::regex expression(regex);
            // Note this could be done much more simply using sregex_iterator, but AZStd::regex_iterator appears to be broken and doesn't compile. (LY-116293)
            AZStd::match_results<AZStd::string::const_iterator> results;
            AZStd::string::const_iterator searchFrom = text.begin();
            do
            {
                if (AZStd::regex_search<AZStd::string::const_iterator>(searchFrom, text.end(), results, expression))
                {
                    count++;
                    searchFrom = results[0].second;
                }
                else
                {
                    searchFrom = text.end();
                }
            } while (searchFrom != text.end());
            return count;
        }
    
        AZStd::string BuildFileNameWithExtension(const AZStd::string& shaderSourceFile,
                                                                   const AZStd::string& tempFolder,
                                                                   const char* outputExtension)
        {
            AZStd::string outputFile;
            AzFramework::StringFunc::Path::GetFileName(shaderSourceFile.c_str(), outputFile);
            AzFramework::StringFunc::Path::Join(tempFolder.c_str(), outputFile.c_str(), outputFile);
            AzFramework::StringFunc::Path::ReplaceExtension(outputFile, outputExtension);
            return outputFile;
        }

        namespace CommandLineArgumentUtils
        {
            AZStd::vector<AZStd::string> GetListOfArgumentNames(AZStd::string_view commandLineString)
            {
                AZStd::vector<AZStd::string> listOfTokens;
                AzFramework::StringFunc::Tokenize(commandLineString, listOfTokens, " \t\n");
                AZStd::vector<AZStd::string> listOfArguments;
                for (const AZStd::string& token : listOfTokens)
                {
                    AZStd::vector<AZStd::string> splitArguments;
                    AzFramework::StringFunc::Tokenize(token, splitArguments, "=");
                    listOfArguments.push_back(splitArguments[0]);
                }
                return listOfArguments;
            }

            AZStd::string RemoveArgumentsFromCommandLineString(
                AZStd::array_view<AZStd::string> listOfArgumentsToRemove, AZStd::string_view commandLineString)
            {
                AZStd::string customizedArguments = commandLineString;
                for (const AZStd::string& azslcArgumentName : listOfArgumentsToRemove)
                {
                    AZStd::string regexStr = AZStd::string::format("%s(=\\S+)?", azslcArgumentName.c_str());
                    AZStd::regex replaceRegex(regexStr, AZStd::regex::ECMAScript);
                    customizedArguments = AZStd::regex_replace(customizedArguments, replaceRegex, "");
                }
                return customizedArguments;
            }

            AZStd::string RemoveExtraSpaces(AZStd::string_view commandLineString)
            {
                AZStd::vector<AZStd::string> argumentList;
                AzFramework::StringFunc::Tokenize(commandLineString, argumentList, " \t\n");
                AZStd::string cleanStringWithArguments;
                AzFramework::StringFunc::Join(cleanStringWithArguments, argumentList.begin(), argumentList.end(), " ");
                return cleanStringWithArguments;
            }

            AZStd::string MergeCommandLineArguments(AZStd::string_view left, AZStd::string_view right)
            {
                auto listOfArgumentNamesFromRight = GetListOfArgumentNames(right);
                auto leftWithRightArgumentsRemoved = RemoveArgumentsFromCommandLineString(listOfArgumentNamesFromRight, left);
                AZStd::string combinedArguments = AZStd::string::format("%s %s", leftWithRightArgumentsRemoved.c_str(), right.data());
                return RemoveExtraSpaces(combinedArguments);
            }

            bool HasMacroDefinitions(AZStd::string_view commandLineString)
            {
                const AZStd::regex macroRegex(R"((^-D\s*(\w+))|(\s+-D\s*(\w+)))", AZStd::regex::ECMAScript);

                AZStd::smatch match;
                if (AZStd::regex_search(commandLineString.data(), match, macroRegex))
                {
                    return (match.size() >= 1);
                }
                return false;
            }
        } //namespace CommandLineArgumentUtils
    } // namespace RHI
} // namespace AZ
