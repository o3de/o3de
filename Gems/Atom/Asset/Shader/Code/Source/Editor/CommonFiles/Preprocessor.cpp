/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>

#include <CommonFiles/Preprocessor.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/API/ApplicationAPI.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Debug/TraceContext.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

namespace AZ
{
    namespace ShaderBuilder
    {
        /**
        * Capture inclusion directives from the preprocessor so we can
        * pass dependency information to the asset processor.
        */

        void PreprocessorOptions::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PreprocessorOptions>()
                    ->Version(0)
                    ->Field("predefinedMacros", &PreprocessorOptions::m_predefinedMacros)
                    ->Field("projectIncludePaths", &PreprocessorOptions::m_projectIncludePaths)
                    ;
            }
        }

        void PreprocessorOptions::RemovePredefinedMacros(const AZStd::vector<AZStd::string>& macroNames)
        {
            for (const auto& macroName : macroNames)
            {
                m_predefinedMacros.erase(
                    AZStd::remove_if(
                        m_predefinedMacros.begin(), m_predefinedMacros.end(),
                        [&](const AZStd::string& predefinedMacro) {
                            //                                       Haystack,        needle,    bCaseSensitive
                            if (!AZ::StringFunc::StartsWith(predefinedMacro, macroName, true))
                            {
                                return false;
                            }
                            // If found, let's make sure it is not just a substring.
                            if (predefinedMacro.size() == macroName.size())
                            {
                                return true;
                            }
                            // The predefinedMacro can be a string like "macro=value". If we find '=' it is a match.
                            if (predefinedMacro.c_str()[macroName.size()] == '=')
                            {
                                return true;
                            }
                            return false;
                        }),
                    m_predefinedMacros.end());
            }
        }

        ///////////////////////////////////////////////////////////////////////
        // McppBinder starts
        bool McppBinder::StartPreprocessWithCommandLine(int argc, const char* argv[])
        {
            int errorCode = mcpp_lib_main(argc, argv);
            // convert from std::ostringstring to AZStd::string
            m_outputData.code = m_outStream.str().c_str();
            m_outputData.diagnostics = m_errStream.str().c_str();
            return errorCode == 0;
        }

        int McppBinder::Putc_StaticHinge(int c, MCPP_OUTDEST od)
        {
            char asString[2] = { aznumeric_cast<char>(c), 0 };
            return Fputs_StaticHinge(asString, od);
        }

        int McppBinder::Fputs_StaticHinge(const char* s, MCPP_OUTDEST od)
        {
            if (!OkToLog(od))
            {
                return 0;
            }
            // chose the proper stream
            auto& selectedStream = od == MCPP_OUT ? s_currentInstance->m_outStream : s_currentInstance->m_errStream;
            auto tellBefore = selectedStream.tellp();
            // append that message to it
            selectedStream << s;
            return aznumeric_cast<int>(selectedStream.tellp() - tellBefore);
        }

        int McppBinder::Fprintf_StaticHinge(MCPP_OUTDEST od, const char* format, ...)
        {
            if (!OkToLog(od))
            {
                return 0;
            }
            // run the formatting on stack memory first, in case it's enough
            char localBuffer[DefaultFprintfBufferSize];

            va_list args;

            va_start(args, format);
            int count = azvsnprintf(localBuffer, DefaultFprintfBufferSize, format, args);
            va_end(args);

            char* result = localBuffer;

            // @result will be bound to @biggerData in case @localBuffer is not big enough.
            std::unique_ptr<char[]> biggerData;
            // ">=" is the right comparison because in case count == bufferSize
            // We will need an extra byte to accomodate the '\0' ending character.
            if (count >= DefaultFprintfBufferSize)
            {
                // There wasn't enough space in the local store.
                count++; // vsnprintf returns a size that doesn't include the null character.
                biggerData.reset(new char[count]);
                result = &biggerData[0];

                // Remark: for MacOS & Linux it is important to call va_start again before
                // each call to azvsnprintf. Not required for Windows.
                va_start(args, format);
                count = azvsnprintf(result, count, format, args);
                va_end(args);
            }
            else if (count == -1)
            {
                // In Windows azvsnprintf will always return -1 if  @localBuffer is not big enough,
                // But it will write in @localBuffer what it could.
                // See:
                // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/vsnprintf-vsnprintf-vsnprintf-l-vsnwprintf-vsnwprintf-l?view=msvc-160
                // In particular: "If the number of characters to write is greater than count,
                // these functions return -1 indicating that output has been truncated."

                // There wasn't enough space in the local store.
                // Remark: for MacOS & Linux it is important to call va_start again before
                // each call to azvsnprintf. Not required for Windows.
                va_start(args, format);
                count = azvscprintf(format, args);
                count += 1; // vscprintf returns a size that doesn't include the null character.
                va_end(args);

                biggerData.reset(new char[count]);
                result = &biggerData[0];

                va_start(args, format);
                count = azvsnprintf(result, count, format, args);
                va_end(args);
            }

            AZ_Error("Preprocessor", count >= 0, "String formatting of pre-precessor output failed");
            return Fputs_StaticHinge(result, od);
        }

        void McppBinder::IncludeReport_StaticHinge(FILE*, const char*, const char*, const char* path)
        {
            s_currentInstance->m_outputData.includedPaths.insert(path);
        }

        bool McppBinder::OkToLog(MCPP_OUTDEST od)
        {
            bool isErrButOk = od == MCPP_ERR && s_currentInstance->m_plugERR;
            return od == MCPP_OUT || isErrButOk;
        }

        void McppBinder::SetupMcppCallbacks()
        {
            // callback for header included notification
            mcpp_set_report_include_callback(IncludeReport_StaticHinge);
            // callback for output redirection
            mcpp_set_out_func(Putc_StaticHinge, Fputs_StaticHinge, Fprintf_StaticHinge);
        }

        // definitions for the linker
        AZStd::mutex McppBinder::s_mcppExclusiveProtection;
        McppBinder* McppBinder::s_currentInstance = nullptr;

        // McppBinder ends
        ///////////////////////////////////////////////////////////////////////

        // @returns true if a string starts with "-I", which is how the C-preprocessor understands include paths.  Examples:
        // "-Isome/dir" (true)
        // "-I/full/path/" (true)
        // "-DMacro" (false)
        static bool IsArgumentAnIncludeDirectory(const AZStd::string& argument)
        {
            if (argument.size() < 2)
            {
                return false;
            }
            return (argument[0] == '-') && (argument[1] == 'I');
        }

        // Transforms relative include path arguments. If the argument is not an include path the same argument is returned.
        // @param argument A single command line arguments for the C-preprocessor.
        // @param rootDir The root directory that will be joined with the relative path.
        // @returns A copy of @argument if it is NOT an include path argument. If it is a include path then:
        //      - If the directory is an absolute path:
        //          - and it exists, a copy of @argument is returned.
        //          - and it doesn't exist, an empty string is returned.
        //      - If the directory is a relative path, it is transformed into an absolute directory by joining @rootDir with the relative path.
        //          - If the directory exists a string as an include path argument is returned.
        //          - If the directory doesn't exist and empty string is returned.
        static AZStd::string NormalizeIncludePathArgument(const AZStd::string& argument, const AZ::IO::FixedMaxPath& rootDir)
        {
            if (!IsArgumentAnIncludeDirectory(argument))
            {
                return argument;
            }

            AZStd::string includeDirectory = argument.substr(2);
            // Trim spaces at both ends.
            AZ::StringFunc::TrimWhiteSpace(includeDirectory, true, true);
            if (AZ::StringFunc::Path::IsRelative(includeDirectory.c_str()))
            {
                AZ::IO::FixedMaxPath absoluteDirectory = rootDir / includeDirectory;
                if (!AZ::IO::SystemFile::Exists(absoluteDirectory.c_str()))
                {
                    AZ_Error("Preprocessor", false, "The directory argument: %s, doesn't exist as an absolute path: %s. Skipping...",
                        argument.c_str(), absoluteDirectory.c_str());
                    return {};
                }
                return AZStd::string::format("-I%s", absoluteDirectory.c_str());
            }

            // It's an absolute directory. Does it exist?
            if (!AZ::IO::SystemFile::Exists(includeDirectory.c_str()))
            {
                AZ_Error("Preprocessor", false, "The absolute directory argument: %s, doesn't exist as an absolute path: %s. Skipping...",
                    argument.c_str(), includeDirectory.c_str());
                return {};
            }

            // The aboslute directory exists, return the argument as is.
            return argument;
        }

        AZStd::vector<AZStd::string> AppendIncludePathsToArgumentList(const AZStd::vector<AZStd::string>& preprocessorArguments, AZStd::vector<AZStd::string> includePaths)
        {
            auto newList = preprocessorArguments;
            for (const AZStd::string& folder : includePaths)
            {
                newList.push_back(AZStd::string::format("-I%s", folder.c_str()));
            }
            return newList;
        }

        bool PreprocessFile(const AZStd::string& fullPath, PreprocessorData& outputData
            , const AZStd::vector<AZStd::string>& preprocessorArguments
            , bool collectDiagnostics)
        {
            // create a wrapper instance
            McppBinder mcpp(outputData, collectDiagnostics);

            // create the argc/argv
            const char* processName = "builder";
            const char* inputPath = fullPath.c_str();

            AZStd::vector< AZStd::string > argv;
            argv.reserve(2 /*processName + inputPath*/ + preprocessorArguments.size());
            argv.push_back(processName);
            argv.push_back(inputPath);

            // The include directories in C-preprocessor arguments, when relative, are relative
            // to the current project.
            const AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
            for (const auto& cppArgument : preprocessorArguments)
            {
                auto normalizedArgument = NormalizeIncludePathArgument(cppArgument, projectPath);
                if (normalizedArgument.empty())
                {
                    // An empty string means there was an error. The proper message is produced by NormalizePreprocessorIncludePath().
                    return false;
                }
                argv.emplace_back(AZStd::move(normalizedArgument));
            }

            // output the command line:
            AZStd::string stringifiedCommandLine;
            AZ::StringFunc::Join(stringifiedCommandLine, argv.begin(), argv.end(), " ");
            AZ_TracePrintf("Preprocessor", "%s", stringifiedCommandLine.c_str());
            // when we don't specify an -o outfile, mcpp uses stdout.
            // the trick is that since we hijacked putc & puts, stdout will not be written.
            AZStd::vector< const char* > argsOfCstr;
            argsOfCstr.reserve(argv.size() + 1);
            for (const auto& arg : argv)
            {
                argsOfCstr.push_back(&arg[0]);
            }
            argsOfCstr.push_back(nullptr); // usual argv terminator
            bool result = mcpp.StartPreprocessWithCommandLine(aznumeric_cast<int>(argsOfCstr.size()) - 1, argsOfCstr.data());
            return result;
        }

        static void VerifySameFolder([[maybe_unused]] AZStd::string_view path1, [[maybe_unused]] AZStd::string_view path2)
        {
            AZ_Warning("Preprocessing",
                AZ::IO::PathView(path1).ParentPath().LexicallyNormal() == AZ::IO::PathView(path2).ParentPath().LexicallyNormal(),
                "The preprocessed file %.*s is in a different folder than its origin %.*s. Watch for #include problems with relative paths.",
                AZ_STRING_ARG(path1), AZ_STRING_ARG(path2)
            );
        }

        // change/add the #line on all appearances, to fake the origin of the source, to its original file path.
        // Because the asset processor moves source files around, to hack them with binding points, or common headers,
        // and the actual file given to azslc ends up being a temporary, like "filename.azslin.prepend".
        // That file ends up being the real source, obviously. Therefore azslc reports the containing file to be that temp file,
        // for some SRG (any SRG preceding a #line directive). The later job, SRG layout builder, will expect reflected
        // containing file names, to match the ORIGINAL source, and not the actual source in use by azslc.
        // That gymnastic is better for error messages anyway, so instead of making the SRG layout builder more intelligent,
        // we'll fake the origin of the file, by setting the original source as a filename
        //  note that it is not possible to build a file in a different folder and fake it to a file elsewhere because relative includes will fail.
        void MutateLineDirectivesFileOrigin(
            AZStd::string& sourceCode,
            AZStd::string newFileOrigin)
        {
            // don't let backslashes pass, they will cause "token recognition error" in azslc
            AZStd::replace(newFileOrigin.begin(), newFileOrigin.end(), '\\', '/');

            // mcpp has good manners so it inserts a line directive immediately at the beginning.
            // we will use that as the information of the source path to mutate.
            if (sourceCode.starts_with("#line"))
            {
                auto firstQuote = sourceCode.find('"');
                auto secondQuote = firstQuote != AZStd::string::npos ? sourceCode.find('"', firstQuote + 1) : AZStd::string::npos;
                auto originalFile = sourceCode.substr(firstQuote + 1, secondQuote - firstQuote - 1);  // start +1, count -1 because we don't want the quotes included.
                VerifySameFolder(originalFile, newFileOrigin);
                [[maybe_unused]] bool didReplace = AZ::StringFunc::Replace(sourceCode, originalFile.c_str(), newFileOrigin.c_str(), true /*case sensitive*/);
                AZ_Assert(didReplace, "Failed to replace %s for %s in preprocessed source.", originalFile.c_str(), newFileOrigin.c_str());
            }
            else
            {
                AZ_Assert(false, "preprocessed sources must start with #line directive, otherwise it's impossible to auto-detect the original file to mutate.")
            }
        }

        AZStd::vector<AZStd::string> BuildListOfIncludeDirectories([[maybe_unused]] const char* builderName, const char* optionalIncludeFolder)
        {
            AZ_TraceContext("Init include-paths lookup options", "preprocessor");

            AZStd::vector<AZStd::string> includePaths;

            if (optionalIncludeFolder)
            {
                if (AZ::IO::SystemFile::Exists(optionalIncludeFolder))
                {
                    includePaths.emplace_back(AZStd::move(AZ::IO::Path(optionalIncludeFolder).LexicallyNormal().Native()));
                }
            }

            auto PathCompare = [](AZ::IO::PathView searchPath)
            {
                return [searchPath](AZStd::string_view includePathView)
                {
                    return searchPath == AZ::IO::PathView(includePathView);
                };
            };

            // Add the project path to list of include paths
            AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
            if (auto it = AZStd::find_if(includePaths.begin(), includePaths.end(), PathCompare(projectPath));
                it == includePaths.end())
            {
                includePaths.emplace_back(projectPath.c_str(), projectPath.Native().size());
            }

            // get the scan folders of the projects:
            bool success = true;
            AZStd::vector<AZStd::string> scanFoldersVector;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(success,
                &AzToolsFramework::AssetSystemRequestBus::Events::GetScanFolders,
                scanFoldersVector);
            AZ_Warning(builderName, success, "Preprocessor option: Could not acquire a list of scan folders from the database.");

            // but while we transfer to the set, we're going to keep only folders where +/ShaderLib exists
            for (AZ::IO::Path shaderScanFolder : scanFoldersVector)
            {
                shaderScanFolder /= "ShaderLib";
                if (auto it = AZStd::find_if(includePaths.begin(), includePaths.end(), PathCompare(shaderScanFolder));
                    it == includePaths.end())
                {
                    // the folders constructed this fashion constitute the base of automatic include search paths
                    if (AZ::IO::SystemFile::Exists(shaderScanFolder.c_str()))
                    {
                        includePaths.emplace_back(AZStd::move(shaderScanFolder.LexicallyNormal().Native()));
                    }
                }
            }

            // finally the <engineroot>/Gems fallback
            AZ::IO::Path engineGemsFolder(AZStd::string_view{ AZ::Utils::GetEnginePath() });
            engineGemsFolder /= "Gems";
            if (auto it = AZStd::find_if(includePaths.begin(), includePaths.end(), PathCompare(engineGemsFolder));
                it == includePaths.end())
            {
                if (AZ::IO::SystemFile::Exists(engineGemsFolder.c_str()))
                {
                    includePaths.emplace_back(AZStd::move(engineGemsFolder.Native()));
                }
            }

            return includePaths;
        }

    } // namespace ShaderBuilder
} // namespace AZ
