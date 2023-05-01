"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Environmentally-dependent tests for ly_test_tools.cli.codeowners_hint
"""
import os

import ly_test_tools.cli.codeowners_hint as hint


def test_FindCodeowners_ThisCWD_FindForO3DE():
    codeowners_path = hint.find_github_codeowners(os.path.abspath(__file__))
    assert codeowners_path
    assert ".github" in str(codeowners_path), "codeowners file has been tampered with, or test unexpectedly migrated"


def test_FindCodeowners_ThisFilesystemRoot_NotFound():
    codeowners_path = hint.find_github_codeowners(os.path.abspath(os.sep))
    assert not codeowners_path
