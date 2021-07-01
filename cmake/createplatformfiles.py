#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from __future__ import (absolute_import, division,
                        print_function, unicode_literals)

import sys
import json
import os
import subprocess
import argparse
import pathlib

fileContents = ""

def getCopyright():
    return """#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

"""

def getPlatforms():
    return ['Android','iOS','Jasper','Linux','Mac','Provo','Salem','Windows']

def isRestricted(platform):
    return platform in ['Jasper','Provo','Salem']

def createEmptyPlatformFile(platform, rel_path, filename, dev_root, restricted_root):
    full_platform_filename = filename.replace("<platform>", platform.lower())

    if isRestricted(platform):
        full_platform_file_path = restricted_root / platform / rel_path
    else:
        full_platform_file_path = dev_root / rel_path / 'Platform' / platform

    full_platform_file_path.mkdir(parents=True, exist_ok=True)
    full_platform_file_path = (full_platform_file_path / full_platform_filename).resolve()
    alreadyExists = full_platform_file_path.exists()
    if alreadyExists:
        subprocess.run(['p4', 'edit', str(full_platform_file_path)])
    print(f'Creating {full_platform_file_path}')
    with open(full_platform_file_path, 'w') as destination_file:
        destination_file.write(getCopyright())
    if not alreadyExists:
        subprocess.run(['p4', 'add', str(full_platform_file_path)])

def main():
    """script main function"""
    parser = argparse.ArgumentParser(description='This script generates an empty cmake file per platform at a given path\n'
        'Output: will create Platform structure if not present, will create\n'
        '        Platform\<platform>\<filename>_<platformlowercase>.cmake\n'
        '        for each platform.',
        formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('path', type=str, help='Path where the Platform folder is (without including the platform folder) relative to <dev_root>')
    parser.add_argument('filename', type=str, help='Pattern for the platform file (<platform> will be replaced by each platform), e.g. "platform_<platform>.cmake"')
    parser.add_argument('--dev-root', default=os.path.join(os.path.dirname(__file__), '..'), type=str, help='Override where the dev root is.  If none is supplied, it will be auto-detected')
    parser.add_argument('--restricted-root', default='{dev_root}/restricted', type=str, help='Override where the restricted platform root is located.  Defaults to <dev_root>/restricted')
    args = parser.parse_args()

    dev_root = pathlib.Path(args.dev_root).resolve()
    if not dev_root.exists():
        print(f'Dev root at {dev_root} does not exist')
        sys.exit(1)

    restricted_root = pathlib.Path(args.restricted_root.format(**locals())).resolve()

    os.chdir(dev_root)

    input_path = pathlib.Path(args.path).resolve()
    try:
        input_relpath = input_path.relative_to(dev_root)
    except ValueError:
        print(f'Input path, {input_path}, is not a child of dev root, {dev_root}')
        sys.exit(1)

    if not input_path.is_dir():
        print(f'Expected a valid path, got {input_path}')
        sys.exit(1)

    for platform in getPlatforms():
        createEmptyPlatformFile(platform, input_relpath, args.filename, dev_root, restricted_root)

#entrypoint
if __name__ == '__main__':
    main()