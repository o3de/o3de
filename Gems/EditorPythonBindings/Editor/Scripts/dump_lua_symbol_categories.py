#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Dumps the FINAL resolved Script Canvas category path for every reflected class,
# global and EBus into a JSON file that a remote tool (e.g. the VS Code "O3DE
# Development Tools" extension) can JOIN onto the symbol set it scrapes over the
# Script Debug bridge.
#
# The category for each symbol is resolved editor-side by the
# LuaSymbolCategoryReporter (ScriptCanvasEditor gem) using the same three tiers
# the Node Palette uses: translation override -> reflection Category attribute ->
# default bucket. The extension does no resolution; it looks up each scraped
# symbol by identity and uses the path verbatim. A miss = uncategorized.
#
# Run from the Editor:
#   Tools -> Other -> Python Scripts, or from the console:
#   pyRunFile <o3de>/Gems/EditorPythonBindings/Editor/Scripts/dump_lua_symbol_categories.py
# Produces <game_project>\lua_symbol_categories.json (override with --outfile).
#
# Join keys the extension uses:
#   classes           -> typeId   (normalize brace/case on both sides)
#   globalMethods     -> name
#   globalProperties  -> name
#   ebuses            -> name      (senderCategory for Event/Broadcast, handlerCategory for Notification)

import sys
import os
import json
import argparse

import azlmbr.bus as azbus
import azlmbr.script as azscript
import azlmbr.legacy.general as azgeneral


def _dump_class_categories():
    rows = azscript.LuaSymbolCategoryReporterBus(azbus.Broadcast, "GetClassCategories")
    return [
        {"typeId": str(row.typeId), "name": row.name, "category": row.category}
        for row in rows
    ]


def _dump_global_method_categories():
    rows = azscript.LuaSymbolCategoryReporterBus(azbus.Broadcast, "GetGlobalMethodCategories")
    return [{"name": row.name, "category": row.category} for row in rows]


def _dump_global_property_categories():
    rows = azscript.LuaSymbolCategoryReporterBus(azbus.Broadcast, "GetGlobalPropertyCategories")
    return [{"name": row.name, "category": row.category} for row in rows]


def _dump_ebus_categories():
    rows = azscript.LuaSymbolCategoryReporterBus(azbus.Broadcast, "GetEBusCategories")
    return [
        {"name": row.name, "senderCategory": row.senderCategory, "handlerCategory": row.handlerCategory}
        for row in rows
    ]


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Dumps the resolved Script Canvas category path for all classes, globals and EBuses to JSON.")
    parser.add_argument('--outfile', '--o', default='lua_symbol_categories.json',
                        help='Output JSON file. If relative, written under the game project folder.')
    args = parser.parse_args()

    output_file_name = args.outfile
    if not os.path.isabs(output_file_name):
        game_root_path = os.path.normpath(azgeneral.get_game_folder())
        output_file_name = os.path.join(game_root_path, output_file_name)

    payload = {
        "classes": _dump_class_categories(),
        "globalMethods": _dump_global_method_categories(),
        "globalProperties": _dump_global_property_categories(),
        "ebuses": _dump_ebus_categories(),
    }

    try:
        with open(output_file_name, 'wt') as file_obj:
            json.dump(payload, file_obj, indent=2, sort_keys=True)
    except Exception as e:
        print(f"Failed to write {output_file_name}: {e}")
        sys.exit(-1)

    print(f"Lua symbol categories written to: {output_file_name}")
    print(f"  classes={len(payload['classes'])} globalMethods={len(payload['globalMethods'])} "
          f"globalProperties={len(payload['globalProperties'])} ebuses={len(payload['ebuses'])}")
