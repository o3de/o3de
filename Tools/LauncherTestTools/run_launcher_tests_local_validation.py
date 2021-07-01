"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import argparse
import json
import logging
import os
import sys

logger = logging.getLogger(__name__)


def report_error_and_exit(msg):
    """Log an error message and exit with error code 1."""
    logger.error(msg)
    sys.exit(1)

def check_autoexe_cfg(dev_root_folder, project):
    """
    Make sure that the project's autoexec.cfg does not contain a Map command.
    The Launcher Test framework is responsible for loading the map.
    """
    # Open the autoexec.cfg file and read the contents
    autoexec_cfg_path = os.path.join(dev_root_folder, project, 'autoexec.cfg')
    try:
        with open(autoexec_cfg_path) as f:
            content = f.readlines()
    except:
        report_error_and_exit("Failed to read contents of {}".format(autoexec_cfg_path))
    
    # Make sure no map command is detected, the Launcher Test code will be in charge of loading a map
    for line in content:
        if line.lower().startswith('map '):
            report_error_and_exit("Map command '{}' detected in {}".format(line.strip(), autoexec_cfg_path))

def check_gems_enabled(dev_root_folder, project):
    """Check the project's gems to make sure the AutomatedLauncherTesting gem is enabled."""
    # Read the gems.json file
    gems_json_path = os.path.join(dev_root_folder, project, 'gems.json')
    try:
        with open(gems_json_path) as f:
            json_data = json.load(f)
    except:
        report_error_and_exit("Failed to read contents of {}".format(gems_json_path))
    
    # Make sure AutomatedLauncherTesting is enabled
    found = False
    for gem_data in json_data['Gems']:
        if 'AutomatedLauncherTesting' in gem_data['Path']:
            found = True
            break

    if not found:
        report_error_and_exit("Automated Launcer Testing GEM not enabled in {}".format(gems_json_path))

def main():

    parser = argparse.ArgumentParser(description='Run validation on the local environment to check for required Launcher Tests config.')
    parser.add_argument('--dev-root-folder', required=True, help='Path to the root Lumberyard dev folder.')
    parser.add_argument('--project', required=True, help='Lumberyard project.')
    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG)
    
    logger.info("Running validation for project {} ...".format(args.project))
    
    check_autoexe_cfg(args.dev_root_folder, args.project)
    
    check_gems_enabled(args.dev_root_folder, args.project)

    logger.info('Validation complete.')

if __name__== '__main__':
    main()
