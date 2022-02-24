
import os
from pathlib import Path
from box import Box
import logging as _logging
import azpy.config_utils
from azpy.o3de.utils import o3de_utilities as o3de_helpers
from azpy.constants import FRMT_LOG_LONG
from SDK.Python import general_utilities as helpers

"""

convert_material.py

Location:
azpy/o3de/renderer/materials/

Main functions:
- Get all properties material info
- Validate material
- read atom material
- write atom material
- convert legacy material
- convert spec/gloss to metal/rough

"""


def get_dcc_material_properties(dcc_app: str, target_file: Path) -> Box:
    pass


