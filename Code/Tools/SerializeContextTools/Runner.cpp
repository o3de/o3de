/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Settings/CommandLine.h>
#include <Application.h>
#include <Converter.h>
#include <Dumper.h>
#include <SliceConverter.h>

namespace SerializeContextTools
{
    void PrintHelp()
    {
        AZ_Printf("Help", "Serialize Context Tool\n");
        AZ_Printf("Help", "  <action> [-config] [misc options] <action arguments>*\n");
        AZ_Printf("Help", "  [opt] -config=<path>: optional path to application's config file. Default is 'config/editor.xml'.\n");
        AZ_Printf("Help", "  [opt] -specializations=<prefix>: <comma or semicolon>-separated list of optional Registry project\n");
        AZ_Printf("Help", "         specializations, such as 'editor' or 'game' or 'editor;test'.  Default is none. \n");
        AZ_Printf("Help", "\n");
        AZ_Printf("Help", "  'help': Print this help\n");
        AZ_Printf("Help", "    example: 'help'\n");
        AZ_Printf("Help", "\n");
        AZ_Printf("Help", "  'dumpfiles': Dump the content to a .dump.txt file next to the original file.\n");
        AZ_Printf("Help", "    [arg] -files=<path>: ;-separated list of files to verify. Supports wildcards.\n");
        AZ_Printf("Help", "    [opt] -output=<path>: Path to the folder to write to instead of next to the original file.\n");
        AZ_Printf("Help", "    example: 'dumpfiles -files=folder/*.ext;a.ext;folder/another/z.ext'\n");
        AZ_Printf("Help", "\n");
        AZ_Printf("Help", "  'dumpsc': Dump the content of the Serialize and Edit Context to a JSON file.\n");
        AZ_Printf("Help", "    [opt] -output=<path>: Path to the folder to write to instead of next to the original file.\n");
        AZ_Printf("Help", "    example: 'dumpsc -output=../TargetFolder/SerializeContext.json'\n");
        AZ_Printf("Help", "\n");
        AZ_Printf("Help", "  'dumptypes': Dump the list of reflected types to stdout or a file.\n");
        AZ_Printf("Help", "    [opt] --sort=<WORD> : Sorts the reflected type by <WORD> where word can be one of the following values.\n");
        AZ_Printf("Help", R"(          "name", "typeid", "none")" "\n")
        AZ_Printf("Help", "          sorts by name if not specified .\n");
        AZ_Printf("Help", "    [opt] --output-file=<filepath>: Path to the file to output reflected types.\n");
        AZ_Printf("Help", "          If not specfied, output is written to stdout.\n");
        AZ_Printf("Help", R"(    example: 'dumptypes')" "\n");
        AZ_Printf("Help", R"(    example: 'dumptypes --sort=typeid)" "\n");
        AZ_Printf("Help", R"(    example: 'dumptypes --output-file=reflectedtypes.txt)" "\n");
        AZ_Printf("Help", "\n");
        AZ_Printf("Help", "  'convert': Converts a file with an ObjectStream to the new JSON formats.\n");
        AZ_Printf("Help", "    [arg] -files=<path>: <comma or semicolon>-separated list of files to verify. Supports wildcards.\n");
        AZ_Printf("Help", "    [arg] -ext=<string>: Extension to use for the new file.\n");
        AZ_Printf("Help", "    [opt] -dryrun: Processes as normal, but doesn't write files.\n");
        AZ_Printf("Help", "    [opt] -skipverify: After conversion the result will not be compared to the original.\n");
        AZ_Printf("Help", "    [opt] -keepdefaults: Fields are written if a default value was found.\n");
        AZ_Printf("Help", "    [opt] -json-prefix=<prefix>: JSON pointer path prefix to anchor the JSON output underneath.\n");
        AZ_Printf("Help", "           On Windows the <prefix> should be in quotes, as \"/\" is treated as command option prefix\n");
        AZ_Printf("Help", "    [opt] -json-prefix=prefix: Json pointer path prefix to use as a \"root\" for settings.\n");
        AZ_Printf("Help", "    [opt] -verbose: Report additional details during the conversion process.\n");
        AZ_Printf("Help", "    example: 'convert -file=*.slice;*.uislice -ext=slice2'\n");
        AZ_Printf("Help", "\n");
        AZ_Printf("Help", R"(  'convert-ini': Converts windows-style INI file to a json format file.)" "\n");
        AZ_Printf("Help", R"(                 The converted file is suitable for being loaded into the Settings Registry.)" "\n");
        AZ_Printf("Help", R"(                 Can be used to convert .cfg/.ini files.)" "\n");
        AZ_Printf("Help", R"(    [arg] -files=<path...>: <comma or semicolon>-separated list of files to verify. Supports wildcards.)" "\n");
        AZ_Printf("Help", R"(    [opt] -ext=<string>: Extension to use for the new files. default=setreg)" "\n");
        AZ_Printf("Help", R"(    [opt] -dryrun: Processes as normal, but doesn't write files.)" "\n");
        AZ_Printf("Help", R"(    [opt] -json-prefix=<prefix>: JSON pointer path prefix to anchor the JSON output underneath.)" "\n");
        AZ_Printf("Help", R"(           On Windows the <prefix> should be in quotes, as \"/\" is treated as command option prefix)" "\n");
        AZ_Printf("Help", R"(    [opt] -verbose: Report additional details during the conversion process.)" "\n");
        AZ_Printf("Help", R"(    example: 'convert-ini --files=AssetProcessorPlatformConfig.ini;bootstrap.cfg --ext=setreg)" "\n");
        AZ_Printf("Help", "  'convert-slice': Converts ObjectStream-based slice files or legacy levels to a JSON-based prefab.\n");
        AZ_Printf("Help", "    [arg] -files=<path>: <comma or semicolon>-separated list of files to convert. Supports wildcards.\n");
        AZ_Printf("Help", "    [opt] -slices=<path>: <comma or semicolon>-separated list of .slice files that you converted files depends on. Supports wildcards. Use this if you cannot use the asset processor on the target project (like o3de converting lumberyard)\n");
        AZ_Printf("Help", "    [opt] -dryrun: Processes as normal, but doesn't write files.\n");
        AZ_Printf("Help", "    [opt] -keepdefaults: Fields are written if a default value was found.\n");
        AZ_Printf("Help", "    [opt] -verbose: Report additional details during the conversion process.\n");
        AZ_Printf("Help", "    example: 'convert-slice -files=*.slice -specializations=editor\n");
        AZ_Printf("Help", "    example: 'convert-slice -files=Levels/TestLevel/TestLevel.ly -project-path=F:/lmbr-fork/dev/StarterGame -slices=Gems/*.slice -specializations=editor\n");
        AZ_Printf("Help", "\n");
        AZ_Printf("Help", "  'createtype': Create a default constructed object using Json Serialization and output the contents.\n");
        AZ_Printf("Help", "    [arg] --type-name=<string>: Name of type to construct and output.\n");
        AZ_Printf("Help", "          The type must be registered with the Json Registration or Serialize Context.\n");
        AZ_Printf("Help", "          Cannot be specified with the -type-id parameter.\n");
        AZ_Printf("Help", "    [arg] --type-id=<uuid>: Uuid of type to construct and output.\n");
        AZ_Printf("Help", "          The type must be registered with the Json Registration or Serialize Context.\n");
        AZ_Printf("Help", "          Cannot be specified with the -type-name parameter.\n");
        AZ_Printf("Help", "    [opt] --output-file=<filepath>: Path to the file to output constructed object.\n");
        AZ_Printf("Help", "          If not supplied, output is written to stdout.\n");
        AZ_Printf("Help", R"(    [opt] --json-prefix=<prefix>: JSON pointer path prefix to anchor the JSON output underneath.)" "\n");
        AZ_Printf("Help", R"(    example: 'createtype --type-name="AZ::Entity"')" "\n");
        AZ_Printf("Help", R"(    example: 'createtype --type-id="{75651658-8663-478D-9090-2432DFCAFA44}"')" "\n");
        AZ_Printf("Help", R"(    example: 'createtype --type-name="AZ::Entity" --json-prefix="/My/Anchor"')" "\n");
        AZ_Printf("Help", R"(    example: 'createtype --type-name="AZ::Entity" --output-file=object.json)" "\n");
        AZ_Printf("Help", "\n");
        AZ_Printf("Help", "  'createuuid': Create a UUID using a SHA1 hash from a string and output the contents to stdout or a file.\n");
        AZ_Printf("Help", "    [arg] --values=<string...>: One or more strings to convert to UUID.\n");
        AZ_Printf("Help", "        Multiple strings can be specified by either using multiple `--values` option or with a single `--values` option by separating them by a comma without any quotes.\n");
        AZ_Printf("Help", R"(        Ex. --values "engine.json" --values "project.json")" "\n");
        AZ_Printf("Help", R"(        Ex. --values engine.json,project.json)" "\n");
        AZ_Printf("Help", R"(        Ex. --values engine.json,project.json --values gem.json)" "\n");
        AZ_Printf("Help", "    [opt] --values-file=<filepath>: Path to file containing linefeed delimited strings to convert to UUD.\n");
        AZ_Printf("Help", "          specifying an argument of dash '-' reads input from stdin\n");
        AZ_Printf("Help", "    [opt] --output-file=<filepath>: Path to the file to output constructed uuids.\n");
        AZ_Printf("Help", "          If not supplied, output is written to stdout.\n");
        AZ_Printf("Help", "          specifying an argument of dash '-' writes output to stdout\n");
        AZ_Printf("Help", "    [opt] --with-curly-braces=<true|false> Outputs the Uuid with curly braces. Defaults to true\n");
        AZ_Printf("Help", "         Ex. when true = {0123456789abcdef0123456789abcdef}\n");
        AZ_Printf("Help", "         Ex. when false = 0123456789abcdef0123456789abcdef\n");
        AZ_Printf("Help", "    [opt] --with-dashes=<true|false> Outputs the Uuid with dashes. Defaults to true\n");
        AZ_Printf("Help", "         Ex. when true = 01234567-89ab-cdef-0123-456789abcdef\n");
        AZ_Printf("Help", "         Ex. when false = 0123456789abcdef0123456789abcdef\n");
        AZ_Printf("Help", "    [opt] -q --quiet suppress output of string used to generate the Uuid\n");
        AZ_Printf("Help", "         Ex. when set = 01234567-89ab-cdef-0123-456789abcdef\n");
        AZ_Printf("Help", "         Ex. when not set = 01234567-89ab-cdef-0123-456789abcdef <uuid-string>\n");
        AZ_Printf("Help", R"(    example: 'createuuid --values="engine.json"')" "\n");
        AZ_Printf("Help", R"(        output: {3B28A661-E723-5EBE-AB52-EC5829D88C31} engine.json)" "\n");
        AZ_Printf("Help", R"(    example: 'createuuid --values="engine.json" --values="project.json"')" "\n");
        AZ_Printf("Help", R"(        output: {3B28A661-E723-5EBE-AB52-EC5829D88C31} engine.json)" "\n");
        AZ_Printf("Help", R"(        output: {B076CDDC-14DF-50F4-A5E9-7518ABB3E851} project.json)" "\n");
        AZ_Printf("Help", R"(    example: 'createtype --values=engine.json,project.json --output-file=uuids.txt')" "\n");
        AZ_Printf("Help", "\n");
        AZ_Printf("Help", "  Miscellaneous Options:\n");
        AZ_Printf("Help", "  This options can be used with any of the above actions:\n");
        AZ_Printf("Help", "    [opt] --regset <setreg_key>=<setreg_value>: Set setreg_value at key setreg_key within the settings registry.\n");
        AZ_Printf("Help", "    [opt] --project-path <project_path>: Sets the path to the active project. Used to load gems associated with project\n");
    }

    int LaunchSerializeContextTools(int argc, char** argv)
    {
        using namespace AZ::SerializeContextTools;

        const AZ::Debug::Trace tracer;
        constexpr int StdoutDescriptor = 1;
        AZ::IO::FileDescriptorCapturer stdoutCapturer(StdoutDescriptor);

        // Capture command output of command that executed
        // If a failure occured write the output to stderr, otherwise write the output to stdout
        AZStd::string commandOutput;
        auto CaptureStdout = [&commandOutput](AZStd::span<AZStd::byte const> outputBytes)
        {
            commandOutput += AZStd::string_view(reinterpret_cast<const char*>(outputBytes.data()), outputBytes.size());
        };

        stdoutCapturer.Start(CaptureStdout);
        Application application(argc, argv, &stdoutCapturer);
        AZ::ComponentApplication::StartupParameters startupParameters;
        application.Start({}, startupParameters);

        bool result = false;
        const AZ::CommandLine* commandLine = application.GetAzCommandLine();
        bool commandExecuted = false;
        if (commandLine->GetNumMiscValues() >= 1)
        {
            // Set the command executed boolean to true
            commandExecuted = true;
            AZStd::string_view action = commandLine->GetMiscValue(0);
            if (AZ::StringFunc::Equal("dumpfiles", action))
            {
                result = Dumper::DumpFiles(application);
            }
            else if (AZ::StringFunc::Equal("dumpsc", action))
            {
                result = Dumper::DumpSerializeContext(application);
            }
            else if (AZ::StringFunc::Equal("dumptypes", action))
            {
                result = Dumper::DumpTypes(application);
            }
            else if (AZ::StringFunc::Equal("convert", action))
            {
                result = Converter::ConvertObjectStreamFiles(application);
            }
            else if (AZ::StringFunc::Equal("convert-ini", action))
            {
                result = Converter::ConvertConfigFile(application);
            }
            else if (AZ::StringFunc::Equal("convert-slice", action))
            {
                SliceConverter sliceConverter;
                result = sliceConverter.ConvertSliceFiles(application);
            }
            else if (AZ::StringFunc::Equal("createtype", action))
            {
                result = Dumper::CreateType(application);
            }
            else if (AZ::StringFunc::Equal("createuuid", action))
            {
                result = Dumper::CreateUuid(application);
            }
            else
            {
                commandExecuted = false;
            }
        }

        // If a command was not executed, display the help options
        if (!commandExecuted)
        {
            // Stop capture of stdout to allow the help command to output to stdout
            fflush(stdout);
            stdoutCapturer.Stop();
            PrintHelp();
            result = true;
            // Flush stdout stream before restarting the capture to make sure
            // all the help text is output
            fflush(stdout);
            stdoutCapturer.Start(AZStd::move(CaptureStdout));
        }

        if (!result)
        {
            AZ_Printf("SerializeContextTools", "Processing didn't complete fully as problems were encountered.\n");
        }

        application.Stop();

        // Because the FILE* stream is buffered, make sure to flush
        // it before stopping the capture of stdout.
        fflush(stdout);
        stdoutCapturer.Stop();

        // Write out any error output if the command result is non-zero
        if (!result)
        {
            fwrite(commandOutput.data(), 1, commandOutput.size(), stderr);
            return -1;
        }

        return 0;
    }
}
