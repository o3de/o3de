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
    AZ_Printf("Help", "    [opt] -dryrun: Processes as normal, but doesn't write files.\n");
    AZ_Printf("Help", "    [opt] -keepdefaults: Fields are written if a default value was found.\n");
    AZ_Printf("Help", "    [opt] -verbose: Report additional details during the conversion process.\n");
    AZ_Printf("Help", "    example: 'convert-slice -files=*.slice -specializations=editor\n");
    AZ_Printf("Help", "    example: 'convert-slice -files=Levels/TestLevel/TestLevel.ly -specializations=editor\n");
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
    AZ_Printf("Help", "  Miscellaneous Options:\n");
    AZ_Printf("Help", "  This options can be used with any of the above actions:\n");
    AZ_Printf("Help", "    [opt] --regset <setreg_key>=<setreg_value>: Set setreg_value at key setreg_key within the settings registry.\n");
    AZ_Printf("Help", "    [opt] --project-path <project_path>: Sets the path to the active project. Used to load gems associated with project\n");
}

int main(int argc, char** argv)
{
    using namespace AZ::SerializeContextTools;

    const AZ::Debug::Trace tracer;
    constexpr int StdoutDescriptor = 1;
    AZ::IO::FileDescriptorCapturer stdoutCapturer(StdoutDescriptor);

    // Send stdout output to stderr if the executed command returned a failure
    bool suppressStderr = false;
    auto SendStdoutToError = [&suppressStderr](AZStd::span<AZStd::byte const> outputBytes)
    {
        if (!suppressStderr)
        {
            constexpr int StderrDescriptor = 2;
            AZ::IO::PosixInternal::Write(StderrDescriptor, outputBytes.data(), aznumeric_cast<int>(outputBytes.size()));
        }
    };

    stdoutCapturer.Start();
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
        else
        {
            commandExecuted = false;
        }
    }

    // If a command was executed, display the help options
    if (!commandExecuted)
    {
        // Stop capture of stdout to allow the help command to output to stdout
        // stderr messages are suppressed in this case
        fflush(stdout);
        suppressStderr = true;
        stdoutCapturer.Stop(SendStdoutToError);
        PrintHelp();
        result = true;
        // Flush stdout stream before restarting the capture to make sure
        // all the help text is output
        fflush(stdout);
        stdoutCapturer.Start();
    }

    if (!result)
    {
        AZ_Printf("SerializeContextTools", "Processing didn't complete fully as problems were encountered.\n");
    }

    application.Destroy();

    // Write out any stdout to stderr at this point

    // Because the FILE* stream is buffered, make sure to flush
    // it before stopping the capture of stdout.
    fflush(stdout);

    suppressStderr = result;
    stdoutCapturer.Stop(SendStdoutToError);

    return result ? 0 : -1;
}
