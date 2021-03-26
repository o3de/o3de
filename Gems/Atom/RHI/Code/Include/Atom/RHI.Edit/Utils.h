/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include <AzCore/std/string/string.h>
#include <Atom/RHI.Reflect/ShaderStages.h>
#include <Atom/RHI.Edit/ShaderPlatformInterface.h>

namespace AZ
{
    namespace RHI
    {
        //! The shader compiler profiling data.
        //! To keep track of the shader compiler profiling data, we cannot use a shared data structure as multiple processes are running.
        //! Instead, we create JSON files in the temporary job folder, using the process ID for distinct file names.
        struct ShaderCompilerProfiling
        {
            AZ_TYPE_INFO(ShaderCompilerProfiling, "{4DEB54A4-0EB7-4ADF-9229-E9F6724C4D60}");
            AZ_CLASS_ALLOCATOR(ShaderCompilerProfiling, AZ::SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* context);

            //! An entry in the shader compiler profiling data.
            struct Entry
            {
                AZ_TYPE_INFO(ShaderCompilerProfilingEntry, "{DBE390C8-8A06-492A-8494-93AAE0A938E0}");
                AZ_CLASS_ALLOCATOR(Entry, AZ::SystemAllocator, 0);

                static void Reflect(AZ::ReflectContext* context);

                AZStd::string m_executablePath;
                AZStd::string m_parameters;
                float m_elapsedTimeSeconds = 0.0f;
            };

            AZStd::vector<Entry> m_entries;
        };

        static constexpr int Md5NumBytes = 16;
        typedef unsigned char ArrayOfCharForMd5[Md5NumBytes];

        struct PrependArguments
        {
            const char* m_sourceFile;
            const char* m_prependFile;
            const char* m_addSuffixToFileName = nullptr; //!< optional
            const char* m_destinationFolder = nullptr;  //!< optional. if not set, will just use sourceFile's folder
            AZStd::string* m_destinationStringOpt = nullptr;  //!< when not null, PrependFile() will dump the result in that string rather than on disk.
            ArrayOfCharForMd5* m_digest = nullptr; //! optionally run a hash
        };

        //! Prepends prependFile to sourceFile and saves the result in a new file whose path is then returned.
        //! If it fails to prepend the file for any reason, it returns sourceFile's path instead.
        AZStd::string PrependFile(PrependArguments& arguments);

        //! Convert a binary data array to a displayable hexadecimal ascii form.
        //! e.g. "MD5 to string".
        template< size_t N >
        AZStd::string ByteToHexString(const unsigned char (&array)[N])
        {
            AZStd::string result{ N * 2, '\0' };
            for (int i = 0; i < N; ++i)
            {
                char msb = (array[i] >> 4) & 0x0F;
                char lsb = array[i] & 0x0F;
                result[i * 2] = (msb < 0xA ? '0' : 'a') + msb;
                result[i * 2 + 1] = (lsb < 0xA ? '0' : 'a') + lsb;
            }
            return result;
        }

        //! Runs a shader compiler executable with specific parameters.
        //! Returns true it compiled the shader without errors.
        //! Returns false otherwise and with compilation errors messages.
        bool ExecuteShaderCompiler(const AZStd::string& executablePath,
                                   const AZStd::string& parameters,
                                   const AZStd::string& shaderSourcePathForDebug,
                                   const char* toolNameForLog);

        //! Reports error messages to AZ_Error and/or AZ_Warning, given a text blob that potentially contains many lines of errors and warnings.
        //! @param window  Debug window name used for AZ Trace functions
        //! @param errorMessages  String that may contain many lines of errors and warnings
        //! @param return  true if Errors were detected and reported (Warnings don't count)
        bool ReportErrorMessages(AZStd::string_view window, AZStd::string_view errorMessages);

        //! Converts from a RHI::ShaderHardwareStage to an RHI::ShaderStage
        ShaderStage ToRHIShaderStage(ShaderHardwareStage stageType);

        //! Reads a file into a string.
        //! @return the contents of the file as a string, or a failure message if the file couldn't be loaded.
        Outcome<AZStd::string, AZStd::string> LoadFileString(const char* path);

        //! Reads a file into a byte buffer.
        //! @return the contents of the file as a vector, or a failure message if the file couldn't be loaded.
        Outcome<AZStd::vector<uint8_t>, AZStd::string> LoadFileBytes(const char* path);

        //! Returns the number of times a regex appears in some text
        size_t RegexCount(AZStd::string_view text, const char* regex);
    
        //! Returns a string that joins tempFolder with shaderSourceFile and applies  outputExtension as the extension.
        AZStd::string BuildFileNameWithExtension(const AZStd::string& shaderSourceFile,
                                      const AZStd::string& tempFolder,
                                      const char* outputExtension);
    }
}

// collection full range as argument expansion. touch and feel closer to C++20's ranges.
// shortens the line where it's used.
#define AZ_BEGIN_END(collection) AZStd::begin(collection), AZStd::end(collection)
