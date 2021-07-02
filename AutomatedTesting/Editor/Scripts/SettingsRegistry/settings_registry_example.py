#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import azlmbr.settingsregistry as SettingsRegistry
import os

ExampleTestFileSetreg = 'AutomatedTesting/Editor/Scripts/SettingsRegistry/example.file.setreg'
ExampleTestFolderSetreg = 'AutomatedTesting/Editor/Scripts/SettingsRegistry'

def test_settings_registry():
    # Access the Global Settings Registry and dump it to a string
    if SettingsRegistry.g_SettingsRegistry.IsValid():
        dumpedSettings = SettingsRegistry.g_SettingsRegistry.DumpSettings("")
        if dumpedSettings:
            print("Full Settings Registry dumped successfully\n{}", dumpedSettings.value())

    # Making a script local settings registry
    localSettingsRegistry = SettingsRegistry.SettingsRegistry()
    localSettingsRegistry.MergeSettings('''
        {
            "TestObject": {
                "boolValue": false,
                "intValue": 17,
                "floatValue": 32.0,
                "stringValue": "Hello World"
            }
        }''')

    registryVal = localSettingsRegistry.GetBool('/TestObject/boolValue')
    if registryVal:
        print(f"Bool value '{registryVal.value()}' found")

    registryVal = localSettingsRegistry.GetInt('/TestObject/intValue')
    if registryVal:
        print(f"Int value '{registryVal.value()}' found")

    registryVal = localSettingsRegistry.GetFloat('/TestObject/floatValue')
    if registryVal:
        print(f"Float value '{registryVal.value()}' found")

    registryVal = localSettingsRegistry.GetString('/TestObject/stringValue')
    if registryVal:
        print(f"String value '{registryVal.value()}' found")

    if localSettingsRegistry.SetBool('/TestObject/boolValue', True):
        registryVal = localSettingsRegistry.GetBool('/TestObject/boolValue')
        print(f"Bool value '{registryVal.value()}' set")

    if localSettingsRegistry.SetInt('/TestObject/intValue', 22):
        registryVal = localSettingsRegistry.GetInt('/TestObject/intValue')
        print(f"Int value '{registryVal.value()}' set")

    if localSettingsRegistry.SetFloat('/TestObject/floatValue', 16.0):
        registryVal = localSettingsRegistry.GetFloat('/TestObject/floatValue')
        print(f"Float value '{registryVal.value()}' set")

    if localSettingsRegistry.SetString('/TestObject/stringValue', 'Goodbye World'):
        registryVal = localSettingsRegistry.GetString('/TestObject/stringValue')
        print(f"String value '{registryVal.value()}' found")

    if localSettingsRegistry.RemoveKey('/TestObject/stringValue'):
        print("Key '/TestObject/stringValue' has been successfully removed")

    print("current working directory is {}".format(os.getcwd()))

    # Merge a Settings File using the JsonPatch format
    jsonPatchMerged = localSettingsRegistry.MergeSettings('''
        [
            { "op": "add", "path": "/TestObject", "value": {} },
            { "op": "add", "path": "/TestObject/boolValue", "value": false },
            { "op": "add", "path": "/TestObject/intValue", "value": 17 },
            { "op": "add", "path": "/TestObject/floatValue", "value": 32.0 },
            { "op": "add", "path": "/TestObject/stringValue", "value": "Hello World" },
            { "op": "add", "path": "/TestArray", "value": [] },
            { "op": "add", "path": "/TestArray/0", "value": { "intIndex": 3 } },
            { "op": "add", "path": "/TestArray/1", "value": { "intIndex": -55 } }
        ]''', SettingsRegistry.JsonPatch)
        
    if jsonPatchMerged:
        print("JSON in JSON Patch format has been merged successfully to the local settings registry")

    # Below is how the the MergeSettingsFile and MergeSettingsFolder could be used
    if localSettingsRegistry.MergeSettingsFile(ExampleTestFileSetreg):
        print(f"Successfully merged setreg file '{ExampleTestFileSetreg}' to local settings registry")
        registryVal = localSettingsRegistry.GetString('/AutomatedTesting/ScriptingTestArray/3')
        if registryVal:
            print(f"Settings Registry contains '/AutomatedTesting/ScriptingTestArray/3'='{registryVal.value()}' merged from the {ExampleTestFileSetreg}")

    # Add the 'folder' to the Settings Registry so that only non-specialized .setreg
    # and .setreg files which contains only a 'folder' tag are merged into the Setting Registry
    filetags = SettingsRegistry.Specializations()
    filetags.Append('folder') 
    if localSettingsRegistry.MergeSettingsFolder(ExampleTestFolderSetreg, filetags):
        print(f"Successfully merged setreg folder '{ExampleTestFolderSetreg}' to local settings registry")
        registryVal = localSettingsRegistry.GetBool('/AutomatedTesting/Spectra/IsFolder')
        if registryVal:
            print(f"Settings Registry contains '/AutomatedTesting/Spectra/IsFolder'='{registryVal.value()}' merged from the {ExampleTestFolderSetreg} folder")

# Invoke main function
if __name__ == '__main__':
   test_settings_registry()