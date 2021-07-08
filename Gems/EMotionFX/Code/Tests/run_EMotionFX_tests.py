"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

python run_EMotionFX_tests.py --config --vsVersion

Example:
python run_EMotionFX_tests.py --config profile --vsVersion vs2017
python run_EMotionFX_tests.py --config debug --vsVersion vs2019


Requirements:

- Provide a single script that executes all your team's automated BAT/Regression tests

- The script should run the tests successfully if killed in the middle and restarted

- Provide any documentation needed along with the script so that other feature teams wanting to execute your tests can execute them locally

- The documentation should also cover where the test results are reported and how to find failing tests and their logs


"""

import argparse
import os
import subprocess


#Setup Parser
def parser_setup():
    parser = argparse.ArgumentParser(description='Sets up for and runs the specified EMotionFX tests')

    # Configuration
    parser.add_argument('--config', choices=['profile','debug'] , help='The Conrfiguration you have pre-build and want to run the tests on. Options[profile,debug]')

    #Visual Studio Version
    parser.add_argument('--vsVersion', choices=['vs2017','vs2019'], help='The version of Visual Studio you used to build your branch. Options[vs2017, vs2019]')
    


    return parser



#Main Program
def main():
    # Set up CLI arguments for CLI argument parser
    parser = parser_setup()

    # Capture arguments and their values
    args = parser.parse_args()


    # Change directory to branch root
    dev_path = os.path.join(os.path.dirname(__file__), '..', '..','..','..')
    os.chdir(dev_path)
    dirpath = os.getcwd()


    vsVersion = args.vsVersion
    config = args.config

    if vsVersion == 'vs2017':
        if config == 'profile':
            #lmbr_test.cmd scan --dir Bin64vc141.Test --only Gem\.EMotionFX\..*\.dll
            subprocess.run('lmbr_test.cmd scan --dir Bin64vc141.Test --only Gem\.EMotionFX\..*\.dll', check=True)

        elif config == 'debug':
            #lmbr_test.cmd scan --dir Bin64vc141.Debug.Test --only Gem\.EMotionFX\..*\.dll
            subprocess.run('lmbr_test.cmd scan --dir Bin64vc141.Debug.Test --only Gem\.EMotionFX\..*\.dll', check=True)

        else:
            print('INVALID ARGUMENT(s)... Ending')
    elif vsVersion == 'vs2019':
        if config == 'profile':
            #lmbr_test.cmd scan --dir Bin64vc142.Test --only Gem\.EMotionFX\..*\.dll
            subprocess.run('lmbr_test.cmd scan --dir Bin64vc142.Test --only Gem\.EMotionFX\..*\.dll', check=True)

        elif config == 'debug':
            #lmbr_test.cmd scan --dir Bin64vc142.Debug.Test --only Gem\.EMotionFX\..*\.dll
            subprocess.run('lmbr_test.cmd scan --dir Bin64vc141.Debug.Test --only Gem\.EMotionFX\..*\.dll', check=True)

        else:
            print('INVALID ARGUMENT(s)... Ending')
    else:
        print('INVALID ARGUMENT(s)... Ending')
        
    

if __name__ == '__main__':
    main()
