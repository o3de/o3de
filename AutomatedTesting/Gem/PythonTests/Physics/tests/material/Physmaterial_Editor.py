"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import azlmbr.legacy.general as general

from xml.etree import ElementTree

class Physmaterial_Editor:
    """
    This class is used to adjust physmaterial files for use with Open 3D Engine.

    NOTEWORTHY:
        - Must use save_changes() for library modifications to take affect
        - Once file is overwritten there is a small lag before the editor applies these changes. Tests
            must be set up to allow time for this lag.
        - You can use parse() to overwrite the Physmaterial_Editor object with a new file

    Methods:
        - __init__ (self, document_filename = None): Sets up Physmaterial Instance
            - document_filename (type: string): the full path of your physmaterial file
        - parse_file (self): Loads the material library into memory and creates and indexable root object.
        - save_changes (self): Overwrites the contents of the input file with the modified library. Unless
            this is called no changes will occur
        - modify_material (self, material, attribute, value): Modifies a given material. Adjusts values
            if possible, throws errors if not
            - material (type: string): The name of the material, must be exact
            - attribute (type: string): Name of the attribute, must be exact. Restrictions outlined below
            - value (type: string, int, or float): New value for the given attribute. Restrictions
                outlined below
        - delete_material (self, material): Deletes given material from the library.
            - material (type: string): The name of the material, must be exact

    Properties:
        - number_of_materials: Number of materials in the material library

    Input Restrictions:
        - Attribute: Can only be one of the five following values
            - 'Dynamic Friction'
            - 'Static Friction'
            - 'Restitution'
            - 'Friction Combine'
            - 'Restitution Combine'
        - Friction Values: Must be a number either int or float
        - Restitution Values: Must be a number either int or float between 0 and 1
        - Combine Values: Can only be one of the four following values
            - 'Average'
            - 'Minimum'
            - 'Maximum'
            - 'Multiply'

    notes:
    - Due to the setup of material libraries root has a lot of indices that must be used to get to the
        actual library portion. There does not seem to be an easy way to remedy this issue as it makes
        for a difficult rewrite process
    - parse_file must only be called if the file path is not given during initialization.
    """

    def __init__(self, document=None):
        self.document_filename = document
        self.project_folder = general.get_game_folder()
        self._set_path()
        self.parse_file()

    def parse_file(self):
        # type: (str) -> None
        # See if a file exists at the given path
        if not os.path.exists(self.document_filename):
            raise ValueError("Given file, {} ,does not exist".format(self.document_filename))
        # Brings Material Library contents into memory
        try:
            self.dom = ElementTree.parse(self.document_filename)
        except Exception as e:
            print(e)
            raise ValueError('{} not valid'.format(self.document_filename))
        # Turn parsed xml into usable form
        self.root = self.dom.getroot()
        # Check if file is a material library
        asset_typename = self.root[0].get('name') 
        if not asset_typename == "MaterialLibraryAsset":
            if asset_typename:
                print("Given file is a {} file".format(self.root[0].get('name')))
            raise ValueError('File not valid')

    def save_changes(self):
        # type: (None) -> None
        # Over writes file with modified material library contents
        content = ElementTree.tostring(self.root)
        try:
            with open(self.document_filename, "wb") as document:
                document.write(content)
        except Exception as e:
            print(e)
            print("Failed to save changes to script")
        
        # Temporary fix, will need to use OnAssetReloaded callbacks
        general.idle_wait(0.5)

    def delete_material(self, material):
        # type: (str) -> bool
        # Deletes a material from the library
        index = self._find_material_index(material)
        if index != None:
            self.root[0][1].remove(self.root[0][1][index])
            return True
        else:
            print("{} not found in library. No deletion occurred.".format(material))
            return False

    def modify_material(self, material, attribute, value):
        # type: (str, str, float) -> bool
        # Modifies attributes of a given material in the library
        index = self._find_material_index(material)
        attribute_index = Physmaterial_Editor._get_attribute_index(attribute)
        formated_value = Physmaterial_Editor._value_formater(value, 'Restitution' == attribute, 'Combine' in attribute)
        if index != None:
            self.root[0][1][index][0][attribute_index].set('value', formated_value)
            return True
        else:
            print("{} not found in library. No modification of {} occurred.".format(material, attribute))
            return False

    @property
    def number_of_materials(self):
        # type: (str) -> int
        materials = self.root[0][1].findall(".//Class[@name='MaterialFromAssetConfiguration']")
        return len(materials)

    def _set_path(self):
        # type: (str) -> str
        if self.document_filename == None:
            self.document_filename = os.path.join(self.project_folder, "assets", "physics", "surfacetypemateriallibrary.physmaterial")
        else:
            for (root, directories, root_files) in os.walk(self.project_folder):
                for root_file in root_files:
                    if root_file == self.document_filename:
                        self.document_filename = os.path.join(root, root_file)
                        break

    def _find_material_index(self, material):
        # type: (str) -> int
        found = False
        material_index = None
        for index, child in enumerate(self.root[0][1]):
            if child.findall(".//Class[@value='{}']".format(material)):
                if not found:
                    found = True
                    material_index = index
        return material_index

    @staticmethod
    def _value_formater(value, is_restitution, is_combine):
        # type: (float/int/str, bool, bool) -> str
        # Constants
        MIN_RESTITUTION = 0.0000000
        MAX_RESTITUTION = 1.0000000

        if is_combine:
            value = Physmaterial_Editor._get_combine_id(value)
        else:
            if isinstance(value, int) or isinstance(value, float):
                if is_restitution:
                    value = max(min(value, MAX_RESTITUTION), MIN_RESTITUTION)
                value = "{:.7f}".format(value)
            else:
                raise ValueError("Must enter int or float. Entered value was of type {}.".format(type(value)))
        return value

    @staticmethod
    def _get_combine_id(combine_name):
        # type: (str) -> int
        # Maps the Combine mode to its enumerated value used by the Open 3D Engine Editor
        combine_dictionary = {"Average": "0", "Minimum": "1", "Maximum": "2", "Multiply": "3"}
        if combine_name not in combine_dictionary:
            raise ValueError("Invalid Combine Value given. {} is not in combine map".format(combine_name))
        return combine_dictionary[combine_name]

    @staticmethod
    def _get_attribute_index(attribute):
        # type: (str) -> int
        # Maps the attribute names to their corresponding index relative to the line defining the material name.
        attribute_dictionary = {
            "DynamicFriction": 1,
            "StaticFriction": 2,
            "Restitution": 3,
            "FrictionCombine": 4,
            "RestitutionCombine": 5,
        }
        if attribute not in attribute_dictionary:
            raise ValueError("Invalid Material Attribute given. {} is not in attribute map".format(attribute))
        return attribute_dictionary[attribute]
