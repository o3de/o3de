/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Settings/CommandLine.h>
#include <Application.h>
#include <Converter.h>
#include <Dumper.h>
#include <SliceConverter.h>


void PrintHelp()
{
    AZ_Printf("Help", "Serialize Context Tool\n");
    AZ_Printf("Help", "  <action> [-config] <action arguments>*\n");
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
    AZ_Printf("Help", "    example: 'dumpsc -output=../TargetFolder/SerializeContext.json\n");
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
    AZ_Printf("Help", "    example: 'convert -file=*.slice;*.uislice -ext=slice2\n");
    AZ_Printf("Help", "\n");
    AZ_Printf("Help", "  'convertad': Converts an Application Descriptor to the new JSON formats.\n");
    AZ_Printf("Help", "    [opt] -dryrun: Processes as normal, but doesn't write files.\n");
    AZ_Printf("Help", "    [opt] -skipgems: No module entities will be converted and no data will be written to gems.\n");
    AZ_Printf("Help", "    [opt] -skipsystem: No system information is converted and no data will be written to the game registry.\n");
    AZ_Printf("Help", "    [opt] -keepdefaults: Fields are written if a default value was found.\n");
    AZ_Printf("Help", "    [opt] -json-prefix=<prefix>: JSON pointer path prefix to anchor the JSON output underneath.\n");
    AZ_Printf("Help", "           On Windows the <prefix> should be in quotes, as \"/\" is treated as command option prefix\n");
    AZ_Printf("Help", "    [opt] -verbose: Report additional details during the conversion process.\n");
    AZ_Printf("Help", "    [opt] -regset <setreg_key>=<setreg_value>: Set setreg_value at key setreg_key within the settings registry.\n");
    AZ_Printf("Help", "           This can be used for example to override the Active Game Project in the settings registry.\n");
    AZ_Printf("Help", "           instead of using the project_path value from the bootstrap.cfg.\n");
    AZ_Printf("Help", R"(           Ex. -regset "/Amazon/AzCore/Bootstrap/project_path=AutomatedTesting"\n)");
    AZ_Printf("Help", "           This sets the active game project as AutomatedTesting, overrideing the value in the bootstrap.cfg\n");
    AZ_Printf("Help", "    example: 'convertad -config=config/game.xml -dryrun\n");
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
}

int main(int argc, char** argv)
{
    using namespace AZ::SerializeContextTools;

    bool result = false;
    Application application(argc, argv);
    AZ::ComponentApplication::StartupParameters startupParameters;
    application.Start({}, startupParameters);

    const AZ::CommandLine* commandLine = application.GetAzCommandLine();
    if (commandLine->GetNumMiscValues() < 1)
    {
        PrintHelp();
        result = true;
    }
    else
    {
        const AZStd::string& action = commandLine->GetMiscValue(0);
        if (AZ::StringFunc::Equal("dumpfiles", action.c_str()))
        {
            result = Dumper::DumpFiles(application);
        }
        else if (AZ::StringFunc::Equal("dumpsc", action.c_str()))
        {
            result = Dumper::DumpSerializeContext(application);
        }
        else if (AZ::StringFunc::Equal("convert", action.c_str()))
        {
            result = Converter::ConvertObjectStreamFiles(application);
        }
        else if (AZ::StringFunc::Equal("convertad", action.c_str()))
        {
            result = Converter::ConvertApplicationDescriptor(application);
        }
        else if (AZ::StringFunc::Equal("convert-ini", action.c_str()))
        {
            result = Converter::ConvertConfigFile(application);
        }
        else if (AZ::StringFunc::Equal("convert-slice", action.c_str()))
        {
            SliceConverter sliceConverter;
            result = sliceConverter.ConvertSliceFiles(application);
        }
        else
        {
            PrintHelp();
            result = true;
        }
    }

    if (!result)
    {
        AZ_Printf("SerializeContextTools", "Processing didn't complete fully as problems were encountered.\n");
    }

    application.Destroy();

    return result ? 0 : -1;
}
