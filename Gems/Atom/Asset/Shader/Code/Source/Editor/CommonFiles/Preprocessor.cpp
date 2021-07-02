/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>

#define MCPP_DLL_IMPORT 1
#define MCPP_DONT_USE_SHORT_NAMES 1
#include <mcpp_lib.h>
#undef MCPP_DLL_IMPORT

#include <CommonFiles/Preprocessor.h>

#include <AzCore/std/string/string.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Debug/TraceContext.h>

#include <AtomCore/Serialization/Json/JsonUtils.h>

#include <sstream>

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
                            if (!AzFramework::StringFunc::StartsWith(predefinedMacro, macroName, true))
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

        //! Binder helper to Matsui C-Pre-Processor library
        class McppBinder
        {
        public:
            McppBinder(PreprocessorData& out, bool plugERR)
                : m_outputData(out),
                  m_plugERR(plugERR)
            {
                // single live instance
                s_mcppExclusiveProtection.lock();
                s_currentInstance = this;
                SetupMcppCallbacks();
            }
            ~McppBinder()
            {
                s_currentInstance = nullptr;
                s_mcppExclusiveProtection.unlock();
            }

            bool StartPreprocessWithCommandLine(int argc, const char* argv[])
            {
                int errorCode = mcpp_lib_main(argc, argv);
                // convert from std::ostringstring to AZStd::string
                m_outputData.code = m_outStream.str().c_str();
                m_outputData.diagnostics = m_errStream.str().c_str();
                return errorCode == 0;
            }

        private:

            // ====== C-API compatible "Static Hinges" (plain free functions) ======
            // : capturing-lambdas, function-objects, bind-expression; can't be decayed to function pointers,
            // because they hold runtime-dynamic type-erased states. So we need intermediates

            // entry point from mcpp. hijacking its output
            static int Putc_StaticHinge(int c, MCPP_OUTDEST od)
            {
                char asString[2] = { aznumeric_cast<char>(c), 0 };
                return Fputs_StaticHinge(asString, od);
            }

            // entry point from mcpp. hijacking its output
            static int Fputs_StaticHinge(const char* s, MCPP_OUTDEST od)
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

            // entry point from mcpp. hijacking its output
            static int Fprintf_StaticHinge(MCPP_OUTDEST od, const char* format, ...)
            {
                if (!OkToLog(od))
                {
                    return 0;
                }
                // run the formatting on stack memory first, in case it's enough
                constexpr int bufferSize = 256;
                char localBuffer[bufferSize];
                va_list args;
                va_start(args, format);
                int count = azvsnprintf(localBuffer, 256, format, args);
                AZStd::unique_ptr<char[]> biggerData;  // will be bound to a bigger array if necessary.
                char* result = localBuffer;
                if (count > bufferSize)
                {   // there wasn't enough space in the local store.
                    biggerData.reset(new char[count]);
                    result = &biggerData[0];  // change `result`'s pointee
                    count = azvsnprintf(result, count, format, args);
                }
                AZ_Error("Preprocessor", count >= 0, "String formatting of pre-precessor output failed");
                va_end(args);
                return Fputs_StaticHinge(result, od);
            }

            static void IncludeReport_StaticHinge(FILE*, const char*, const char*, const char* path)
            {
                s_currentInstance->m_outputData.includedPaths.insert(path);
            }

            // ====== utility methods =====

            static bool OkToLog(MCPP_OUTDEST od)
            {
                bool isErrButOk = od == MCPP_ERR && s_currentInstance->m_plugERR;
                return od == MCPP_OUT || isErrButOk;
            }

            static void SetupMcppCallbacks()
            {
                // callback for header included notification
                mcpp_set_report_include_callback(IncludeReport_StaticHinge);
                // callback for output redirection
                mcpp_set_out_func(Putc_StaticHinge, Fputs_StaticHinge, Fprintf_StaticHinge);
            }

            // ====== instance data ======
            PreprocessorData& m_outputData;
            std::ostringstream m_outStream, m_errStream;
            bool m_plugERR;

            // ======  shared data  ======
            // MCPP is a library with tons of non TLS global states, it can only be accessed by one client at a time.
            static AZStd::mutex s_mcppExclusiveProtection;
            static McppBinder* s_currentInstance;
        };

        // definitions for the linker
        AZStd::mutex McppBinder::s_mcppExclusiveProtection;
        McppBinder*  McppBinder::s_currentInstance = nullptr;

        bool PreprocessFile(const AZStd::string& fullPath, PreprocessorData& outputData, const PreprocessorOptions& options
            , bool collectDiagnostics, bool preprocessIncludedFiles)
        {
            // create a wrapper instance
            McppBinder mcpp(outputData, collectDiagnostics);

            // create the argc/argv
            const char* processName = "builder";
            const char* inputPath  = fullPath.c_str();
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
            AzFramework::StringFunc::Join(stringifiedCommandLine, argv.begin(), argv.end() - 1, " ");
            AZ_TracePrintf("Preprocessor", "%s", stringifiedCommandLine.c_str());
            // when we don't specify an -o outfile, mcpp uses stdout.
            // the trick is that since we hijacked putc & puts, stdout will not be written. 
            bool result = mcpp.StartPreprocessWithCommandLine(aznumeric_cast<int>(argv.size()) - 1, argv.data());
            return result;
        }

        static void VerifySameFolder(const AZStd::string& path1, const AZStd::string& path2)
        {
            AZStd::string folder1, folder2;
            AzFramework::StringFunc::Path::GetFolderPath(path1.c_str(), folder1);
            AzFramework::StringFunc::Path::GetFolderPath(path2.c_str(), folder2);
            AzFramework::StringFunc::Path::Normalize(folder1);
            AzFramework::StringFunc::Path::Normalize(folder2);
            AZ_Warning("Preprocessing",
                folder1 == folder2,
                "The preprocessed file %s is in a different folder than its origin %s. Watch for #include problems with relative paths.",
                path1.c_str(), path2.c_str()
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
        //  note that it is not possible to build a file in a different folder and fake it to a file eslewhere because relative includes will fail.
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
                auto firstQuote  = sourceCode.find('"');
                auto secondQuote = sourceCode.find('"', firstQuote + 1);
                auto originalFile = sourceCode.substr(firstQuote + 1, secondQuote - firstQuote - 1);  // start +1, count -1 because we don't want the quotes included.
                VerifySameFolder(originalFile, newFileOrigin);
                [[maybe_unused]] bool didReplace = AzFramework::StringFunc::Replace(sourceCode, originalFile.c_str(), newFileOrigin.c_str(), true /*case sensitive*/);
                AZ_Assert(didReplace, "Failed to replace %s for %s in preprocessed source.", originalFile.c_str(), newFileOrigin.c_str());
            }
            else
            {
                AZ_Assert(false, "preprocessed sources must start with #line directive, otherwise it's impossible to auto-detect the original file to mutate.")
            }
        }

        namespace
        {
            template< typename Container1, typename Container2 >
            void TransferContent(Container1& destination, Container2&& source)
            {
                destination.insert(AZStd::end(destination),
                                   AZStd::make_move_iterator(AZStd::begin(source)),
                                   AZStd::make_move_iterator(AZStd::end(source)));
            }

            void DeleteFromSet(const AZStd::string& string, AZStd::set<AZStd::string>& set)
            {
                auto iter = set.find(string);
                if (iter != set.end())
                {
                    set.erase(iter);
                }
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

            // we transfer to a set, to order the folders, uniquify them, and ensure deterministic build behavior
            AZStd::set<AZStd::string> scanFoldersSet;
            // Add the project path to list of include paths
            AZ::IO::FixedMaxPathString projectPath = AZ::Utils::GetProjectPath();
            scanFoldersSet.emplace(projectPath.c_str(), projectPath.size());
            if (optionalIncludeFolder)
            {
                scanFoldersSet.emplace(optionalIncludeFolder, strnlen(optionalIncludeFolder, AZ::IO::MaxPathLength));
            }
            // but while we transfer to the set, we're going to keep only folders where +/ShaderLib exists
            for (AZStd::string folder : scanFoldersVector)
            {
                AzFramework::StringFunc::Path::Join(folder.c_str(), "ShaderLib", folder);
                if (AZ::IO::SystemFile::Exists(folder.c_str()))
                {
                    scanFoldersSet.emplace(std::move(folder));
                }
            } // the folders constructed this fashion constitute the base of automatic include search paths

            // get the engine root:
            AZ::IO::FixedMaxPath engineRoot = AZ::Utils::GetEnginePath();

            // add optional additional options
            for (AZStd::string& path : options.m_projectIncludePaths)
            {
                path = (engineRoot / path).String();
                DeleteFromSet(path, scanFoldersSet);  // no need to add a path two times.
            }
            // back-insert the default paths (after the config-read paths we just read)
            TransferContent(/*to:*/options.m_projectIncludePaths, /*from:*/scanFoldersSet);
            // finally the <engineroot>/Gems fallback
            AZStd::string gemsFolder;
            AzFramework::StringFunc::Path::Join(engineRoot.c_str(), "Gems", gemsFolder);
            options.m_projectIncludePaths.push_back(gemsFolder);
        }

    } // namespace ShaderBuilder
} // namespace AZ
