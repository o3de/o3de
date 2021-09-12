#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import sys
import os

import azlmbr.bus as azbus
import azlmbr.script as azscript
import azlmbr.legacy.general as azgeneral

def _dump_class_symbol(class_symbol):
    """
    class_symbol of type azlmbr.lua.ClassSymbol
    """
    print(f"** {class_symbol}")
    print("Properties:")
    for property_symbol in class_symbol.properties:
        print(f"   - {property_symbol}")
    print("Methods:")
    for method_symbol in class_symbol.methods:
        print(f"   - {method_symbol}")

def _dump_lua_classes():
    class_symbols = azscript.SymbolsReporterBus(azbus.Broadcast,
                                           "GetListOfClasses")
    print("========  Classes ==========")
    sorted_classes_by_named = sorted(class_symbols, key=lambda class_symbol: class_symbol.name)
    for class_symbol in sorted_classes_by_named:
        _dump_class_symbol(class_symbol)
        print("\n\n")

def _dump_global_property(property_symbol):
    """
    class_symbol of type azlmbr.lua.ClassSymbol
    """
    print(f"** {class_symbol}")
    print("Properties:")
    for property_symbol in class_symbol.properties:
        print(f"   - {property_symbol}")
    print("Methods:")
    for method_symbol in class_symbol.methods:
        print(f"   - {method_symbol}")

def _dump_lua_globals():
    global_properties = azscript.SymbolsReporterBus(azbus.Broadcast,
                                           "GetListOfGlobalProperties")
    print("========  Global Properties ==========")
    sorted_properties_by_name = sorted(global_properties, key=lambda symbol: symbol.name)
    for property_symbol in sorted_properties_by_name:
        print(f"- {property_symbol}")
    print("\n\n")
    global_functions = azscript.SymbolsReporterBus(azbus.Broadcast,
                                           "GetListOfGlobalFunctions")
    print("========  Global Functions ==========")
    sorted_functions_by_name = sorted(global_functions, key=lambda symbol: symbol.name)
    for function_symbol in sorted_functions_by_name:
        print(f"- {function_symbol}")
    print("\n\n")    


def _dump_lua_ebus(ebus_symbol):
    print(f">> {ebus_symbol}")
    sorted_senders = sorted(ebus_symbol.senders, key=lambda symbol: symbol.name)
    for sender in sorted_senders:
        print(f"  - {sender}")
    print("\n")


def _dump_lua_ebuses():
    ebuses = azscript.SymbolsReporterBus(azbus.Broadcast,
                                           "GetListOfEBuses")
    print("========  Ebus List ==========")
    sorted_ebuses_by_name = sorted(ebuses, key=lambda symbol: symbol.name)
    for ebus_symbol in sorted_ebuses_by_name:
        _dump_lua_ebus(ebus_symbol)
    print("\n\n")


if __name__ == "__main__":
    redirecting_stdout = False
    orig_stdout = sys.stdout
    if len(sys.argv) > 1:
        output_file_name = sys.argv[1]
        if not os.path.isabs(output_file_name):
            game_root_path = os.path.normpath(azgeneral.get_game_folder())
            output_file_name = os.path.join(game_root_path, output_file_name)
        try:
            fileObj = open(output_file_name, 'wt')
            sys.stdout = fileObj
            redirecting_stdout = True
        except:
            print(f"Failed to open {output_file_name}")
            sys.exit(-1)

    what_to_do = []
    if len(sys.argv) > 2:
        what_to_do.append(sys.argv[2].lower())
    if len(sys.argv) > 3:
        what_to_do.append(sys.argv[3].lower())
    if len(sys.argv) > 4:
        what_to_do.append(sys.argv[4].lower())

    if len(what_to_do) < 1:
        what_to_do = ("c", "g", "e")

    for action in what_to_do:
        if action == "c":
            _dump_lua_classes()
        elif action == "g":
            _dump_lua_globals()
        elif action == "e":
            _dump_lua_ebuses()
    
    if redirecting_stdout:
        sys.stdout.close()
        sys.stdout = orig_stdout
        print(f" Symbols Are available in: {output_file_name}")