/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>

#include <CommonFiles/Preprocessor.h>

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

        bool PreprocessFile(const AZStd::string& fullPath, PreprocessorData& outputData, const PreprocessorOptions& options
            , bool collectDiagnostics, bool preprocessIncludedFiles)
        {
            // create a wrapper instance
            McppBinder mcpp(outputData, collectDiagnostics);

            // create the argc/argv
            const char* processName = "builder";
            const char* inputPath = fullPath.c_str();
            // let's create the equivalent of that expression but in dynamic form:
            //const char* argv[] = { processName, szInPath, "-C", "-+", "-D macro1"..., "-I path"..., NULL };
            AZStd::vector< const char* > argv;
            argv.reserve(5 + options.m_predefinedMacros.size() * 2 + options.m_projectIncludePaths.size() * 2);
            argv.push_back(processName);
            argv.push_back(inputPath);
            if (!preprocessIncludedFiles)
            {
                argv.push_back("-z");
            }
            argv.push_back("-C");  // conserve comments
            argv.push_back("-+");  // C++ mode
            for (const AZStd::string& macroDef : options.m_predefinedMacros)
            {
                argv.push_back("-D");
                argv.push_back(&macroDef[0]); // pointers to the string data will be stable for the duration of the call
            }
            for (const AZStd::string& folder : options.m_projectIncludePaths)
            {
                argv.push_back("-I");
                argv.push_back(&folder[0]);  // pointers to the string data will be stable for the duration of the call
            }
            argv.push_back(nullptr); // usual argv terminator
            // output the command line:
            AZStd::string stringifiedCommandLine;
            AZ::StringFunc::Join(stringifiedCommandLine, argv.begin(), argv.end() - 1, " ");
            AZ_TracePrintf("Preprocessor", "%s", stringifiedCommandLine.c_str());
            // when we don't specify an -o outfile, mcpp uses stdout.
            // the trick is that since we hijacked putc & puts, stdout will not be written. 
            bool result = mcpp.StartPreprocessWithCommandLine(aznumeric_cast<int>(argv.size()) - 1, argv.data());
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

        // populate options with scan folders and contents of parsing shader_global_build_options.json
        void InitializePreprocessorOptions(
            PreprocessorOptions& options, [[maybe_unused]] const char* builderName, const char* optionalIncludeFolder)
        {
            AZ_TraceContext("Init include-paths lookup options", "preprocessor");

            // get the scan folders of the projects:
            bool success = true;
            AZStd::vector<AZStd::string> scanFoldersVector;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(success,
                &AzToolsFramework::AssetSystemRequestBus::Events::GetScanFolders,
                scanFoldersVector);
            AZ_Warning(builderName, success, "Preprocessor option: Could not acquire a list of scan folders from the database.");

            // Add the project path to list of include paths
            AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
            auto FindPath = [](AZ::IO::PathView searchPath)
            {
                return [searchPath](AZStd::string_view includePathView)
                {
                    return searchPath == AZ::IO::PathView(includePathView);
                };
            };
            if (auto it = AZStd::find_if(options.m_projectIncludePaths.begin(), options.m_projectIncludePaths.end(), FindPath(projectPath));
                it == options.m_projectIncludePaths.end())
            {
                options.m_projectIncludePaths.emplace_back(projectPath.c_str(), projectPath.Native().size());
            }
            if (optionalIncludeFolder)
            {
                if (auto it = AZStd::find_if(options.m_projectIncludePaths.begin(), options.m_projectIncludePaths.end(), FindPath(optionalIncludeFolder));
                    it == options.m_projectIncludePaths.end())
                {
                    if (AZ::IO::SystemFile::Exists(optionalIncludeFolder))
                    {
                        options.m_projectIncludePaths.emplace_back(AZStd::move(AZ::IO::Path(optionalIncludeFolder).LexicallyNormal().Native()));
                    }
                }
            }
            // but while we transfer to the set, we're going to keep only folders where +/ShaderLib exists
            for (AZ::IO::Path shaderScanFolder : scanFoldersVector)
            {
                shaderScanFolder /= "ShaderLib";
                if (auto it = AZStd::find_if(options.m_projectIncludePaths.begin(), options.m_projectIncludePaths.end(), FindPath(shaderScanFolder));
                    it == options.m_projectIncludePaths.end())
                {
                    // the folders constructed this fashion constitute the base of automatic include search paths
                    if (AZ::IO::SystemFile::Exists(shaderScanFolder.c_str()))
                    {
                        options.m_projectIncludePaths.emplace_back(AZStd::move(shaderScanFolder.LexicallyNormal().Native()));
                    }
                }
            }

            // finally the <engineroot>/Gems fallback
            AZ::IO::Path engineGemsFolder(AZStd::string_view{ AZ::Utils::GetEnginePath() });
            engineGemsFolder /= "Gems";
            if (auto it = AZStd::find_if(options.m_projectIncludePaths.begin(), options.m_projectIncludePaths.end(), FindPath(engineGemsFolder));
                it == options.m_projectIncludePaths.end())
            {
                if (AZ::IO::SystemFile::Exists(engineGemsFolder.c_str()))
                {
                    options.m_projectIncludePaths.emplace_back(AZStd::move(engineGemsFolder.Native()));
                }
            }
        }

    } // namespace ShaderBuilder
} // namespace AZ
