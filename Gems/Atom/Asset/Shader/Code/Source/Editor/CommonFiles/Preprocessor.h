/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/RTTIMacros.h>

#define MCPP_DLL_IMPORT 1
#define MCPP_DONT_USE_SHORT_NAMES 1
#include <mcpp_lib.h>
#undef MCPP_DLL_IMPORT

#include <sstream>

namespace UnitTest
{
    class McppBinderTests;
}

namespace AZ
{
    class ReflectContext;

    namespace ShaderBuilder
    {
        //! Collects data output from the PreprocessFile() function
        struct PreprocessorData
        {
            //! Will contain the preprocessed code.
            AZStd::string code;

            //! May contain warning and error messages, if this option is enabled in PreprocessFile().
            AZStd::string diagnostics;

            //! Will contain the entire inclusion tree, not just the files include by top level AZSL file.
            AZStd::set<AZStd::string> includedPaths;
        };

        //! Object to store preprocessor options, as will be passed in the command line.
        struct PreprocessorOptions final
        {
            AZ_RTTI(PreprocessorOptions, "{684181FC-7372-49FC-B69C-1FF510A29621}");
            AZ_CLASS_ALLOCATOR(PreprocessorOptions, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* context);

            //! passed as -I folder1 -I folder2...
            //! folders are relative to the dev folder of the project
            AZStd::vector<AZStd::string> m_projectIncludePaths;

            //! Each string is of the type "name[=value]"
            //! passed as -Dmacro1[=value1] -Dmacro2 ... to MCPP.
            AZStd::vector<AZStd::string> m_predefinedMacros;

            //! Removes all macros from @m_predefinedMacros that appear in @macroNames
            void RemovePredefinedMacros(const AZStd::vector<AZStd::string>& macroNames);

            //! if needed, we may add configurations like
            //!   "keep comments" or "don't predefine non-standard macros"
            //!   or "output diagnostics to std.err" or "enable digraphs/trigraphs"...
        };

        //! @param builderName: Used for debugging.
        //! @param optionalIncludeFolder: If not null, will be added at the beginning of the returned list of include folders.
        //! @returns A list of fully qualified directory paths that will be given to the c-preprocessor to find the included files in .azsl files.
        AZStd::vector<AZStd::string> BuildListOfIncludeDirectories(const char* builderName, const char* optionalIncludeFolder = nullptr);

        // @returns A new list of command arguments for the C-Preprocessor where each string in @includePaths
        //     is appended to @preprocessorArguments as "-I<path>".
        AZStd::vector<AZStd::string> AppendIncludePathsToArgumentList(const AZStd::vector<AZStd::string>& preprocessorArguments, AZStd::vector<AZStd::string> includePaths);

        /**
        * Runs the preprocessor on the given source file path, and stores results in outputData.
        * @param fullPath   The file to preprocess
        * @param outputData Collects data from the preprocessor. This will be filled out as much as possible, even if preprocessing fails.
        * @param preprocessorArguments The command line arguments for the C-preprocessor.
        * @param collectDiagnostics If true, warnings and errors will be collected in outputData.diagnostics instead of using AZ_Warning and AZ_Error.
        * @return false if the preprocessor failed
        */
        bool PreprocessFile(
            const AZStd::string& fullPath,
            PreprocessorData& outputData,
            const AZStd::vector<AZStd::string>& preprocessorArguments,
            bool collectDiagnostics = false);

        //! Replace all ocurrences of #line "whatever" by #line "whatwewant"
        void MutateLineDirectivesFileOrigin(
            AZStd::string& sourceCode,
            AZStd::string newFileOrigin);

        //! Binder helper to Matsui C-Pre-Processor library
        class McppBinder
        {
        public:
            McppBinder(PreprocessorData& out, bool plugERR)
                : m_outputData(out)
                , m_plugERR(plugERR)
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

            // This constant is in the header so McppBinderTests can see it.
            static constexpr int DefaultFprintfBufferSize = 256;

            bool StartPreprocessWithCommandLine(int argc, const char* argv[]);

        private:
            friend class ::UnitTest::McppBinderTests;

            // ====== C-API compatible "Static Hinges" (plain free functions) ======
            // : capturing-lambdas, function-objects, bind-expression; can't be decayed to function pointers,
            // because they hold runtime-dynamic type-erased states. So we need intermediates

            // entry point from mcpp. hijacking its output
            static int Putc_StaticHinge(int c, MCPP_OUTDEST od);

            // entry point from mcpp. hijacking its output
            static int Fputs_StaticHinge(const char* s, MCPP_OUTDEST od);

            // entry point from mcpp. hijacking its output
            static int Fprintf_StaticHinge(MCPP_OUTDEST od, const char* format, ...);

            static void IncludeReport_StaticHinge(FILE*, const char*, const char*, const char* path);

            // ====== utility methods =====

            static bool OkToLog(MCPP_OUTDEST od);

            static void SetupMcppCallbacks();

            // ====== instance data ======
            PreprocessorData& m_outputData;
            std::ostringstream m_outStream, m_errStream;
            bool m_plugERR;

            // ======  shared data  ======
            // MCPP is a library with tons of non TLS global states, it can only be accessed by one client at a time.
            static AZStd::mutex s_mcppExclusiveProtection;
            static McppBinder* s_currentInstance;
        };

    } // ShaderBuilder
} // AZ
