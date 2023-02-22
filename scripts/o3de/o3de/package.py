#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import os
import platform
import argparse
import json
import logging
import pathlib
import sys
import subprocess
import shutil
import pathlib
import json
import importlib.util
from o3de import utils, manifest, validation, repo


logger = logging.getLogger('o3de.print_registration')
logging.basicConfig(format=utils.LOG_FORMAT)

SUCCESS = 0

#goal: simple CLI to package and build/automate what we need. This is proven with the prototype, so we're good to start RFC.
# we will need sufficient time to flesh out the RFC, then the engineering plan. Otherwise it fees.
# thankfully, I'm not blocking anyone since this is an automation task
# can I ship this before RFC is accepted (feature flags? or separate branch) talk with Joe if this is acceptable for smallest MVP
# all this being said, it is better that I do not overengineer at  this point. Better to write the RFC/design, get feedback, and adjust accordingly. Make sure to supply as many relevant considerations as possible

#design review - ask joe about this, or if he still wants me to write RFC first. (do the minimum important things. Let the design sheperd guide you)

#review automated test code for running executables
#issue with timeouts: this can be extremely difficult
#if someone clones a mature project...
# SPIKE: figure out good system for timeouts. It would suck if it stalls out, but it also sucks if it prematurely times out
# how do we show progress?
# how do we handle logging?

#add parameters for if we want to expose logging or not? otherwise render progress?
def process_command(cmd_args, project_path):
    ret = SUCCESS
    process = subprocess.Popen(cmd_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd = project_path)
    while process.poll() is None:
        line = process.stdout.readline()
        if not line: break
        print(line.decode('ascii'))
    
    #flush out any remaining text from stdout
    print(process.stdout.read().decode('ascii'))

    stderr = process.stderr.read()

    process.kill()

    if bool(process.returncode):
        ret  = process.returncode
        print("ERROR OCCURRED " + str(ret))
        print(stderr.decode('ascii'))

    return ret


def package_hi():
    print("This is hello from o3de/packaging.py!")
    return 0

def package_none():
    print("It seems like packaging is not supported for your platform or use case...")
    return 0

#this is a stand alone proof of concept test function, which just proves it's possible to automate building in Python
#we will use the remainder of the code to work out the framework we would like to use to funnel building procedures
#algorithm here is based on main documentation page for monolithic builds:
#https://www.o3de.org/docs/user-guide/packaging/windows-release-builds/#debugging

#QUESTIONS:
#1) how do we go about detecting the correct asset processor batch executable? (do we fetch from vs_2022, or vs_2019?)
#2) how do we formulaically specify the correct cmake generator?
#3) what commands should we run to pre-build our executables? the o3de docs seem to indicate doing it from the project folder, but @stankowi provides commands from the engine folder
#4) how do we standardize rules related to the packaging rules file found in the project? What parameters should go in there?

#should we break this into smaller steps?
# preliminary list:
# step 1: validation - can we proceed?
# step 2: preparation - asset processing, etc.
# step 3: c++ build - actual build of c++ code with config
# step 4: bundle content for use
# step 5: prep non c++/non asset content 
# step 6: prep directory layout with all prepared content
# step 7: backup then post validation
# step 8: putting everything into container for distribution (zip file, apk file, ipa file)
#enable code re-use. This could also inform _how_ we setup our keys

# we can rely on making this code file expandable as a fallback
# we should provide legacy warnings as code updates progress

# how do we handle with engine customer/contributor
# RFC will help inform my logistical questions on what needs to be split and when
# test driven development would help

WINDOWS_PRE_ARGS = ['powershell.exe']

def package_windows_standalone_monolithic(project_path: str,
                                          engine_path:str,
                                          zipped: bool):
    result = 0

    #packaging rules should be in a standard location with respect to the project. Note, if this is not found, we need to have reasonable defaults
    PROJECT_PATH = project_path
    PACKAGING_RULES_PATH = os.path.join(PROJECT_PATH, "packaging",  "packaging_config.packagerules")

    packaging_rules = None
    with open(PACKAGING_RULES_PATH, "r") as prules_file:
        packaging_rules = json.load(prules_file)
        print(packaging_rules)

    #note: we need ability to supply a default option!
    ENGINE_PATH = packaging_rules["engine-path"]
    THIRD_PARTY_PATH  = packaging_rules["third-party-path"]
    CMAKE_GENERATOR = packaging_rules["cmake-generator"]
    CMAKE_CONFIG = packaging_rules["cmake-config"]
    BUILD_MODE = packaging_rules["project-build-mode"]

    if "zip-name" in packaging_rules:
        zipped_name = packaging_rules["zip-name"]
    else:
        zipped_name = "Monolithic"

    if "zip-folder-path" in packaging_rules:
        zipped_folder = packaging_rules["zip-folder-path"]
    else:
        zipped_folder = ""

    #check automated tests for how we discover executables and output
    #hard coded paths _must_ be checked, also check for timeouts
    ASSET_PROCESSOR_BATCH_EXE = os.path.join(ENGINE_PATH, 'build\\windows_vs2022\\bin\\profile\\AssetProcessorBatch.exe')
    ASSET_PROCESSOR_CMD_ARGS = [ASSET_PROCESSOR_BATCH_EXE, '--platform=pc', f'--project-path={PROJECT_PATH}']

    #build the asset processor batch if not done so yet

    print("build all relevant tools first")
    
    ENGINE_BUILD_CMD_ARGS = ['cmake', '--build', 'build/windows_vs2022', '--target' ,'Editor', '--target', 'AssetProcessorBatch', '--config', 'profile', '--', '-m']
    if process_command(WINDOWS_PRE_ARGS + ENGINE_BUILD_CMD_ARGS, ENGINE_PATH) != SUCCESS:
        print("Prebuilding Engine tools failed!")
        return 1

    print("run the command for the asset processor")
    if process_command(ASSET_PROCESSOR_CMD_ARGS, PROJECT_PATH) != SUCCESS:
        print("Asset Processing Failed!")
        return 1
    

    print("get cmake to prep the project")
    CMAKE_PREP_CMD_ARGS = ['cmake', '-B', BUILD_MODE, '-S','.','-G',CMAKE_GENERATOR,
                            f'-DLY_3RDPARTY_PATH={THIRD_PARTY_PATH}',
                            '-DLY_MONOLITHIC_GAME=1']
    if process_command(WINDOWS_PRE_ARGS + CMAKE_PREP_CMD_ARGS, PROJECT_PATH) != SUCCESS:
        print("CMake Build Prep Failed!")
        return 1
    
    #C:\workspace\projects\NewspaperDeliveryGame\install\bin\Windows\release\Monolithic

    print("get cmake to build the project to install directory")
    CMAKE_RELEASE_CMD_ARGS = ['cmake', '--build', BUILD_MODE, '--target', 'install', '--config', CMAKE_CONFIG]
    if process_command(CMAKE_RELEASE_CMD_ARGS, PROJECT_PATH) != SUCCESS:
        print("CMake Build Release Failed!")
        return 1
    
    if zipped:
        print("Zipping contents...")
        #zip the final file
        # shutil.make_archive("newspaper_delivery_game", "zip", os.path.join(PROJECT_PATH, "install", "bin", "Windows", CMAKE_CONFIG, "Monolithic"))


        build_package_base_path = os.path.join(PROJECT_PATH, "install", "bin", "Windows", CMAKE_CONFIG)
        build_package_path = os.path.join(build_package_base_path, "Monolithic")

        shutil.make_archive(build_package_path, "zip", build_package_base_path, "Monolithic")
        
        #determine which folder to use for zipped file
        if os.path.isdir(zipped_folder) and zipped_folder != '':
            zip_dest = zipped_folder
        else:
            zip_dest = build_package_base_path
            
        print("moving zipped archive to "+zip_dest)
        shutil.move(os.path.join(build_package_base_path, "Monolithic.zip"), os.path.join(zip_dest, zipped_name+".zip"))

    print("finished packaging")
    return result


#this is where we can run the switchboard logic for determining the right packaging function to run
#the first layer recognizes what platform we're running packaging from. This may require OS specific prep-logic before running packaging logic
def _run_packaging(args: argparse) -> int:
    replacements = dict()
    engine_path = manifest.get_project_engine_path(args.project_path)
    replacements["${EnginePath}"] = str(engine_path)
    replacements["${ProjectPath}"] = str(args.project_path)
    replacements["${ThirdPartyPath}"] = "C:\\workspace\\o3de-packages"

    with open(args.export_rules_file, 'r') as erf:
        workflow_json = json.load(erf)

        workflow_rules = workflow_json.get("workflow") 
        assert workflow_rules is not None, "workflow is not specified in json file!"

        for rule in workflow_rules:
            
            if "comment" in rule:
                continue
            elif "print" in rule:
                print(rule["print"])
            elif "step" in rule:
                #perform necessary replacements
                print("Running Step: "+rule["step"])
                for key, value in replacements.items():
                    if "executable" in rule:
                        rule["executable"] = rule["executable"].replace(key, value)
                    for ai in enumerate(rule["args"]):
                        arg = rule["args"][ai[0]]
                        arg = arg.replace(key, value)
                        rule["args"][ai[0]] = arg
                    if "cwd" in rule:
                        rule["cwd"] = rule["cwd"].replace(key, value)
                
                step_result_replacement_name = "result-from-"+rule["step"]

                cwd = rule.get("cwd", args.project_path)
                
                if rule.get("using-python", False):
                    external_script_path = rule["executable"]
                    module_name = os.path.basename(external_script_path)

                    sys.path.insert(0, os.path.split(pathlib.Path(external_script_path))[0])
                    sys.path.insert(0, os.getcwd())

                    spec = importlib.util.spec_from_file_location(module_name, external_script_path)
                    module = importlib.util.module_from_spec(spec)
                    sys.modules[module_name] = module
                    module.o3de_project_path = args.project_path
                    module.o3de_engine_path  = engine_path
                    spec.loader.exec_module(module)

                    step_result = 1 #todo: determine module failure later

                else:
                    arglist = WINDOWS_PRE_ARGS+ [rule["executable"]] + rule["args"]

                    step_result = process_command(arglist, cwd)

                quit_on_fail = rule.get("quit-on-fail", False)

                if step_result != SUCCESS and quit_on_fail:
                    print(f'WORKFLOW FAILED AT STEP {rule["step"]}')
                    return 1
                    
                replacements[step_result_replacement_name] = str(step_result)
        print("finished running workflow!")

    return 0


def add_parser_args(parser):
    #if no use case is specified, we assume standalone
    use_case_group = parser.add_mutually_exclusive_group(required=False)
    # use_case_group.add_argument('-s', '--standalone', action='store_true',
    #                    default=True,
    #                    help='builds as standalone.')
    
    #need to get terminology termed out - i.e. packaging.py can be extremely confusing.
    #need to get dictionary standardized, check with @stankowi on this, need steps and terms fully defined

    #no need to future proof if there's a good off ramp. If our code is easily modifiable, the customer can handle the edge cases
    #for iterative builds, make sure they are not trapped in a dead end. Need Asset bundling addressed

    #something to also think about: what would a long term GUI look like for this?
    #GUI can output config files and commands issued in packaging/build process. Robust code echo mode like in Blender

    #try to sit down with carbonated and cyanide for their build processes. They need thrad obfuscation and boundaries, anti-cheat, modding, etc.

    #simulation customers: two challenges
    #1) live-feed/simulation and human interface use cases. deploying to dedicated enviroments
    #2) they don't need as much as the game teams, but it could conflict with the pipeline.

    #

    # parser.add_argument('-m', '--monolithic', action='store_true',
    #                    default=False,
    #                    help='bundles as monolithic game release.')
    # parser.add_argument('-z', '--zipped', action='store_true',
    #                     default=False,
    #                     help='takes the packaged build and zips it')
    parser.add_argument('-erf', '--export-rules-file', type=pathlib.Path, required=True, help='Export rules workflow JSON file')
    parser.add_argument('-pp', '--project-path', type=pathlib.Path, required=True,
                        help='Project to package')
    # parser.add_argument('-ep', '--engine-path', type=pathlib.Path, required=True,
    #                     help='Engine for building tooling')
    parser.set_defaults(func=_run_packaging)
    

def add_args(subparsers):
    """
    add_args is called to add subparsers arguments to each command such that it can be
    a central python file such as o3de.py.
    It can be run from the o3de.py script as follows
    call add_args and execute: python o3de.py register-show --engine-projects
    :param subparsers: the caller instantiates subparsers and passes it in here
    """

    packaging_subparser = subparsers.add_parser('packaging')
    add_parser_args(packaging_subparser)


def main():
    """
    Runs packaging.py script as standalone script
    """
    # parse the command line args
    the_parser = argparse.ArgumentParser()


    # add args to the parser
    add_parser_args(the_parser)

    # parse args
    the_args = the_parser.parse_args()

    # run
    ret = the_args.func(the_args) if hasattr(the_args, 'func') else 1

    # return
    sys.exit(ret)


if __name__ == "__main__":
    main()