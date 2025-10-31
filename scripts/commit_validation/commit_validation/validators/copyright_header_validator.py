#
# Copyright (c) Contributors to the Open 3D Engine Project
# For complete copyright and license terms please see the LICENSE at the root of this distribution.

# SPDX-License-Identifier: Apache-2.0 OR MIT#
#

import fnmatch
import os
import re
from typing import Type, List

from commit_validation.commit_validation import Commit, CommitValidator, IsFileSkipped, SOURCE_AND_SCRIPT_FILE_EXTENSIONS, EXCLUDED_VALIDATION_PATTERNS, VERBOSE

OPEN_3D_ENGINE_PATTERN = re.compile(r'copyright[\s]*(?:\(c\))?[\s]*.*?Contributors\sto\sthe\sOpen\s3D\sEngine\sProject\.\sFor\scomplete\scopyright\sand\slicense\sterms\splease\ssee\sthe\sLICENSE\sat\sthe\sroot\sof\sthis\sdistribution\.', re.IGNORECASE | re.DOTALL)
OPEN_3D_ENGINE_PATTERN_L1 = re.compile(r'copyright[\s]*(?:\(c\))?[\s]*.*?Contributors\sto\sthe\sOpen\s3D\sEngine\sProject', re.IGNORECASE | re.DOTALL)
OPEN_3D_ENGINE_PATTERN_L2 = re.compile(r'For\scomplete\scopyright\sand\slicense\sterms\splease\ssee\sthe\sLICENSE\sat\sthe\sroot\sof\sthis\sdistribution', re.IGNORECASE | re.DOTALL)

AMAZON_ORIGINAL_COPYRIGHT_PATTERN = re.compile(r'.*?this\sfile\sCopyright\s*\(c\)\s*Amazon\.com.*?', re.IGNORECASE | re.DOTALL)
AMAZON_MODIFICATION_COPYRIGHT_PATTERN = re.compile(r'.*?Modifications\scopyright\sAmazon\.com', re.IGNORECASE | re.DOTALL)
CRYTEK_COPYRIGHT_PATTERN = re.compile(r'Copyright Crytek', re.MULTILINE)

EXCLUDED_COPYRIGHT_VALIDATION_PATTERNS = [
    '*/Code/Framework/AzCore/AzCore/Math/Sha1.h',                                       # Copyright 2007 Andy Tompkins.
    '*/Code/Framework/AzCore/AzCore/std/string/utf8/core.h',                            # Copyright 2006 Nemanja Trifunovic
    '*/Code/Framework/AzCore/AzCore/std/string/utf8/unchecked.h',                       # Copyright 2006 Nemanja Trifunovic
    '*/Gems/Atom/Feature/Common/Assets/ShaderLib/Atom/Features/PBR/Lights/Ltc.azsli',   # Copyright (c) 2017, Eric Heitz, Jonathan Dupuy, Stephen Hill and David Neubelt.
    '*/Code/Legacy/CryCommon/MTPseudoRandom.h',                                         # Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura
    '*/Code/Framework/AzQtComponents/AzQtComponents/Components/FlowLayout.*'            # Copyright (C) 2015 The Qt Company Ltd.
] + EXCLUDED_VALIDATION_PATTERNS

THIS_FILE = os.path.normcase(__file__)

class CopyrightHeaderValidator(CommitValidator):
    """A file-level validator that makes sure a file contains the standard copyright header"""

    def run(self, commit: Commit, errors: List[str]) -> bool:
        for file_name in commit.get_files():
            if os.path.normcase(file_name) == THIS_FILE:
                # Skip this validator file
                continue
            for pattern in EXCLUDED_COPYRIGHT_VALIDATION_PATTERNS:
                if fnmatch.fnmatch(file_name, pattern):
                    if VERBOSE: print(f'{file_name}::{self.__class__.__name__} SKIPPED - Validation pattern excluded on path.')
                    break
            else:
                if IsFileSkipped(file_name):
                    if VERBOSE: print(f'{file_name}::{self.__class__.__name__} SKIPPED - File excluded based on extension.')
                    continue

                has_o3de_pattern_line_1 = False
                has_o3de_pattern_line_2 = False
                has_o3de_pattern_single_line = False
                has_amazon_mod_pattern = False
                has_crytek_pattern = False
                has_original_amazon_copyright_pattern = False
                has_stale_o3de_pattern = False

                with open(file_name, 'rt', encoding='utf8', errors='replace') as fh:
                    for line in fh:
                        if OPEN_3D_ENGINE_PATTERN.search(line):
                            has_o3de_pattern_single_line = True
                        elif OPEN_3D_ENGINE_PATTERN_L1.search(line):
                            has_o3de_pattern_line_1 = True
                        elif OPEN_3D_ENGINE_PATTERN_L2.search(line):
                            has_o3de_pattern_line_2 = True
                        elif AMAZON_ORIGINAL_COPYRIGHT_PATTERN.search(line):
                            has_original_amazon_copyright_pattern = True
                        elif CRYTEK_COPYRIGHT_PATTERN.search(line):
                            has_crytek_pattern = True
                        elif AMAZON_MODIFICATION_COPYRIGHT_PATTERN.search(line):
                            has_amazon_mod_pattern = True

                if has_original_amazon_copyright_pattern:
                    # Has the original Lumberyard Amazon copyright, has not been updated to the O3DE copyright
                    error_message = str(f'{file_name}::{self.__class__.__name__} FAILED - Source file has legacy Amazon Lumberyard copyright.')
                    if VERBOSE: print(error_message)
                    errors.append(error_message)

                if has_amazon_mod_pattern:
                    # Has the Modifications Amazon notice
                    error_message = str(f'{file_name}::{self.__class__.__name__} FAILED - Source file contains Modifications copyright Amazon.com.')
                    if VERBOSE: print(error_message)
                    errors.append(error_message)

                if has_o3de_pattern_line_1 and not has_o3de_pattern_line_2:
                    # Has the stale the O3DE copyright (without the 'For complete copyright...')
                    error_message = str(f"{file_name}::{self.__class__.__name__} FAILED - Source file O3DE copyright header missing 'For complete copyright...'")
                    if VERBOSE: print(error_message)
                    errors.append(error_message)

                if not has_o3de_pattern_single_line and not has_o3de_pattern_line_1 and not has_o3de_pattern_line_2:
                    # Missing the O3DE copyright AND does not have the Amazon Modifications copyright, assuming that this file is missing valid copyrights in general
                    error_message = str(f'{file_name}::{self.__class__.__name__} FAILED - Source file missing O3DE copyright header.')
                    if VERBOSE: print(error_message)
                    errors.append(error_message)

                if has_crytek_pattern:
                    error_message = str(f'{file_name}::{self.__class__.__name__} FAILED - Source file contains legacy CryTek original file copyright. Must be deleted.')
                    if VERBOSE: print(error_message)
                    errors.append(error_message)

                if not has_o3de_pattern_single_line and not has_o3de_pattern_line_1 and not has_o3de_pattern_line_2 and not has_original_amazon_copyright_pattern and not has_crytek_pattern and not has_amazon_mod_pattern:
                    error_message = str(f'{file_name}::{self.__class__.__name__} FAILED - Source file missing any recognized copyrights.')
                    if VERBOSE: print(error_message)
                    errors.append(error_message)


        return (not errors)


def get_validator() -> Type[CopyrightHeaderValidator]:
    """Returns the validator class for this module"""
    return CopyrightHeaderValidator
