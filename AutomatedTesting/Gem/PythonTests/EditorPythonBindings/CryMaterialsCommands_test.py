"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

#
# This is a pytest module to test the in-Editor Python API from ViewPane.h
#
import pytest
pytest.importorskip('ly_test_tools')

import sys
import os
sys.path.append(os.path.dirname(__file__))
from hydra_utils import launch_test_case


@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize('launcher_platform', ['windows_editor'])
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['auto_test'])
class TestLegacyCryMaterialsCommandsAutomation(object):

    def test_Legacy_CryMaterials(self, request, editor, level, launcher_platform):

        unexpected_lines=[]
        expected_lines = [
            # "Material Settings/Shader updated correctly", # Disabled, SPEC-3590
            # "Material Settings/Surface Type updated correctly", # Disabled, SPEC-3590
            "Texture Maps/Diffuse/Tiling/IsTileU updated correctly",           
            "Texture Maps/Diffuse/Tiling/TileU updated correctly",
            "Texture Maps/Diffuse/Rotator/Type updated correctly",
            "Texture Maps/Diffuse/Rotator/Amplitude updated correctly",
            "Texture Maps/Diffuse/Oscillator/AmplitudeU updated correctly",
            "Opacity Settings/Opacity updated correctly",
            "Opacity Settings/AlphaTest updated correctly",
            "Opacity Settings/Additive updated correctly",
            "Lighting Settings/Diffuse Color updated correctly",
            "Lighting Settings/Specular Color updated correctly",
            "Lighting Settings/Emissive Intensity updated correctly",
            "Lighting Settings/Emissive Color updated correctly",
            "Advanced/Allow layer activation updated correctly",
            "Advanced/2 Sided updated correctly",
            "Advanced/No Shadow updated correctly",
            "Advanced/Use Scattering updated correctly",
            "Advanced/Hide After Breaking updated correctly",
            "Advanced/Fog Volume Shading Quality High updated correctly",
            "Advanced/Blend Terrain Color updated correctly",
            "Advanced/Voxel Coverage updated correctly",
            "Advanced/Propagate Opacity Settings updated correctly",
            "Advanced/Propagate Lighting Settings updated correctly",
            "Advanced/Propagate Advanced Settings updated correctly",
            "Advanced/Propagate Texture Maps updated correctly",
            "Advanced/Propagate Shader Params updated correctly",
            "Advanced/Propagate Shader Generation updated correctly",
            "Advanced/Propagate Vertex Deformation updated correctly",
            # "Shader Params/Blend Factor updated correctly", # Disabled, SPEC-3590
            # "Shader Params/Indirect bounce color updated correctly", # Disabled, SPEC-3590
            "Vertex Deformation/Type updated correctly",
            "Vertex Deformation/Wave Length X updated correctly",
            "Vertex Deformation/Wave X/Level updated correctly",
            "Vertex Deformation/Wave X/Amplitude updated correctly",
            "Vertex Deformation/Wave X/Phase updated correctly",
            "Vertex Deformation/Wave X/Frequency updated correctly"
            ]
        
        test_case_file = os.path.join(os.path.dirname(__file__), 'CryMaterialsCommands_test_case.py')
        launch_test_case(editor, test_case_file, expected_lines, unexpected_lines)

