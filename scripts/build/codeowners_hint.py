"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

A commandline interface tool to inspect GitHub CODEOWNERS files
"""
import sys
import ly_test_tools.cli.codeowners_hint

if __name__ == '__main__':
    sys.exit(ly_test_tools.cli.codeowners_hint._main())
