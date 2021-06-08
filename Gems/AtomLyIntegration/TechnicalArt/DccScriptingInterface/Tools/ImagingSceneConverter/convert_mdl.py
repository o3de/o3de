# coding:utf-8
# !/usr/bin/python
#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

# File Description:
# This is the main entry point of the Legacy Material Conversion scripts. It provides
# UI for the script as well as coordinating many of its core processes. The purpose
# of the conversion scripts is to provide a way for individuals and game teams to
# convert assets and materials previously built for legacy spec/gloss based implementation
# to the current PBR metal rough formatting, with the new '.material' files
# replacing the previous '.mtl' descriptions. The script also creates Maya working files
# with the FBX files present and adds Stingray materials for preview as well as further
# look development
# -------------------------------------------------------------------------

# Usage: Maya Shelf Button code- Must use Maya O3DE Launcher
#
# tools_path = os.path.join(os.environ['LY_PROJECT_PATH'], 'Tools')
# if tools_path not in sys.path:
#     sys.path.append(tools_path)
#
# from ImagingSceneConverter import convert_mdl
# reload(convert_mdl)
# convert_mdl.run()


# -- Standard Python modules --
import logging as _logging
import shutil
import socket
import sys
import os


# -- Third Party --
from PIL import Image
from PySide2 import QtWidgets, QtCore
from PySide2.QtCore import QObject
from shiboken2 import wrapInstance
from maya import OpenMayaUI as omui


# --------------------------------------------------------------------------
# -- Global Definitions --
_MODULENAME = 'Tools.ImagingSceneConverter.convert_mdl'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOCAL_HOST = socket.gethostbyname(socket.gethostname())
# -------------------------------------------------------------------------


class Convert(QtWidgets.QWidget):
    def __init__(self):
        super(Convert, self).__init__()
        self.texture_types = ['Base_Color', 'Rough_Height', 'Normal_Metallic']
        self.setWindowFlags(self.windowFlags() & ~QtCore.Qt.WindowContextHelpButtonHint)
        self.target_folder = None
        self.start_conversion()

    def start_conversion(self):
        input_dialog = QtWidgets.QInputDialog()
        input_dialog.setWindowFlags(self.windowFlags() & ~QtCore.Qt.WindowContextHelpButtonHint)
        text, ok = input_dialog.getText(self, 'Set Path', 'Enter ASIN directory path')
        if ok:
            _LOGGER.info('Dialog ok: Path: {}'.format(text))
            self.target_folder = os.path.abspath(text)
            if os.path.isdir(self.target_folder):
                self.convert_imaging_mdl_textures()

    def convert_imaging_mdl_textures(self):
        for item in os.listdir(self.target_folder):
            basename = os.path.splitext(item)[0]
            for texture_type in self.texture_types:
                if basename.endswith(texture_type):
                    texture_outputs = self.get_texture_outputs(texture_type)
                    _LOGGER.info('Texture: {}   Outputs: {}'.format(basename, texture_outputs))
                    self.convert_textures(os.path.join(self.target_folder, item), texture_type, texture_outputs)
        _LOGGER.info('Conversion Complete.')

    def convert_textures(self, file_path, texture_type, texture_outputs):
        for index, output_texture in enumerate(texture_outputs):
            destination_file = file_path.replace(texture_type, output_texture)
            destination_path = os.path.join(self.target_folder, destination_file)
            buffer = self.get_color_channels(file_path) if index == 0 else self.get_alpha_channel(file_path)
            buffer.save(destination_path)
        self.move_old_file(file_path)


    def get_texture_outputs(self, texture_type):
        if texture_type ==  'Base_Color':
            output = ['BaseColor', 'Alpha']
        elif texture_type == 'Rough_Height':
            output = ['Roughness', 'Height']
        else:
            output = ['Normal', 'Metallic']
        return output

        
    def get_color_channels(self, file_path):
        im = Image.open(file_path).convert('RGBA')
        r, g, b, a = im.split()
        color_image = Image.merge('RGB', (r, g, b))
        return color_image
    
    
    def get_alpha_channel(self, file_path):
        im = Image.open(file_path).convert('RGBA')
        alpha = im.split()[-1]
        bg = Image.new("RGBA", im.size, (0, 0, 0, 255))
        bg.paste(alpha, mask=alpha)
        alpha_image = bg.convert('L')
        return alpha_image
        
        
    def move_old_file(self, src):
        archive_folder = os.path.join(self.target_folder, 'archive')
        if not os.path.exists(archive_folder):
            os.makedirs(archive_folder)
        dst = os.path.join(archive_folder, os.path.basename(src))
        shutil.move(src, dst)


def run():
    Convert()
