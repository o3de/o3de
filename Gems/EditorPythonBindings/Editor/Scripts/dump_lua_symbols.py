#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# The easiest way to run this script in O3DE
# is to run it via the "Python Scripts" window in the Editor:
# Tools -> Other -> Python Scripts.
# Will produce the file <game_project>\lua_symbols.txt
#
# Alternatively from the Editor console:
# pyRunFile C:\GIT\o3de\Gems\EditorPythonBindings\Editor\Scripts\dump_lua_symbols.py
# Will produce the file <game_project>\lua_symbols.txt
#
# or if you need to customize the name of the output file
# pyRunFile C:\GIT\o3de\Gems\EditorPythonBindings\Editor\Scripts\dump_lua_symbols.py -o my_lua_name.txt
# Will produce the file <game_project>\my_lua_name.txt

# This script shows basic usage of the LuaSymbolsReporterBus,
# Which can be used to report all symbols available for
# game scripting with Lua.

import sys
import os
import argparse
from typing import TextIO

import azlmbr.bus as azbus
import azlmbr.script as azscript
import azlmbr.legacy.general as azgeneral


def _dump_class_symbol(file_obj: TextIO, class_symbol: azlmbr.script.LuaClassSymbol):
    file_obj.write(f"** {class_symbol}\n")
    file_obj.write("Properties:\n")
    for property_symbol in class_symbol.properties:
        file_obj.write(f"   - {property_symbol}\n")
    file_obj.write("Methods:\n")
    for method_symbol in class_symbol.methods:
        file_obj.write(f"   - {method_symbol}\n")


def _dump_lua_classes(file_obj: TextIO):
    class_symbols = azscript.LuaSymbolsReporterBus(azbus.Broadcast,
                                           "GetListOfClasses")
    file_obj.write("========  Classes ==========\n")
    sorted_classes_by_named = sorted(class_symbols, key=lambda class_symbol: class_symbol.name)
    for class_symbol in sorted_classes_by_named:
        _dump_class_symbol(file_obj, class_symbol)
        file_obj.write("\n\n")


def _dump_lua_globals(file_obj: TextIO):
    global_properties = azscript.LuaSymbolsReporterBus(azbus.Broadcast,
                                           "GetListOfGlobalProperties")
    file_obj.write("========  Global Properties ==========\n")
    sorted_properties_by_name = sorted(global_properties, key=lambda symbol: symbol.name)
    for property_symbol in sorted_properties_by_name:
        file_obj.write(f"- {property_symbol}\n")
    file_obj.write("\n\n")
    global_functions = azscript.LuaSymbolsReporterBus(azbus.Broadcast,
                                           "GetListOfGlobalFunctions")
    file_obj.write("========  Global Functions ==========\n")
    sorted_functions_by_name = sorted(global_functions, key=lambda symbol: symbol.name)
    for function_symbol in sorted_functions_by_name:
        file_obj.write(f"- {function_symbol}\n")
    file_obj.write("\n\n")    


def _dump_lua_ebus(file_obj: TextIO, ebus_symbol: azlmbr.script.LuaEBusSymbol):
    file_obj.write(f">> {ebus_symbol}\n")
    sorted_senders = sorted(ebus_symbol.senders, key=lambda symbol: symbol.name)
    for sender in sorted_senders:
        file_obj.write(f"  - {sender}\n")
    file_obj.write("\n\n")


def _dump_lua_ebuses(file_obj: TextIO):
    ebuses = azscript.LuaSymbolsReporterBus(azbus.Broadcast,
                                           "GetListOfEBuses")
    file_obj.write("========  Ebus List ==========\n")
    sorted_ebuses_by_name = sorted(ebuses, key=lambda symbol: symbol.name)
    for ebus_symbol in sorted_ebuses_by_name:
        _dump_lua_ebus(file_obj, ebus_symbol)
    file_obj.write("\n\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Dumps All ebuses, classes and global symbols available for Lua scripting.')
    parser.add_argument('--outfile', '--o', default='lua_symbols.txt',
                    help='output file file where all the symbols will be dumped to. If relative, will be under the game project folder.')
    parser.add_argument('--all', '--a', default=True, action='store_true',
                    help='If true dumps all symbols to the outfile. Equivalent to specifying --c --g --e')
    parser.add_argument('--classes', '--c', default=False, action='store_true',
                    help='If true dumps Class symbols.')
    parser.add_argument('--globals', '--g', default=False, action='store_true',
                    help='If true dumps Global symbols.')
    parser.add_argument('--ebuses', '--e', default=False, action='store_true',
                    help='If true dumps Ebus symbols.')

    args = parser.parse_args()
    output_file_name = args.outfile
    if not os.path.isabs(output_file_name):
        game_root_path = os.path.normpath(azgeneral.get_game_folder())
        output_file_name = os.path.join(game_root_path, output_file_name)
    try:
        file_obj = open(output_file_name, 'wt')
    except Exception as e:
        print(f"Failed to open {output_file_name}: {e}")
        sys.exit(-1)

    if args.classes:
        _dump_lua_classes(file_obj)
    
    if args.globals:
        _dump_lua_globals(file_obj)

    if args.ebuses:
        _dump_lua_ebuses(file_obj)

    if (not args.classes) and (not args.globals) and (not args.ebuses):
        _dump_lua_classes(file_obj)
        _dump_lua_globals(file_obj)
        _dump_lua_ebuses(file_obj)

    file_obj.close()
    print(f" Lua Symbols Are available in: {output_file_name}")
