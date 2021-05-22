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

import os
import json
import subprocess
import re
import git_utils
from git_utils import Repo
from enum import Enum

def print_in_color(txt_msg, fore_tupple, back_tupple,):
    #prints the text_msg in the foreground color specified by fore_tupple with the background specified by back_tupple 
    #text_msg is the text, fore_tupple is foregroud color tupple (r,g,b), back_tupple is background tupple (r,g,b)
    rf,gf,bf=fore_tupple
    rb,gb,bb=back_tupple
    msg='{0}' + txt_msg
    mat='\33[38;2;' + str(rf) +';' + str(gf) + ';' + str(bf) + ';48;2;' + str(rb) + ';' +str(gb) + ';' + str(bb) +'m' 
    print(msg .format(mat))
    print('\33[0m') # returns default print color to back to black

class SequenceType(Enum):
    REGULAR = 1
    TEST_IMPACT_ANALYSIS = 2

class TestImpact:
    def __init__(self, config_file, dst_commit):
        with open(config_file, "r") as config_data:
            config = json.load(config_data)
            self.__repo_dir = config["repo"]["root"]
            self.__source_of_truth = config["repo"]["source_of_truth"]
            self.__historic_workspace = config["workspace"]["historic"]["root"]
            self.__temp_workspace = config["workspace"]["temp"]["root"]
            self.__active_workspace = config["workspace"]["active"]["root"]
            last_commit_hash_path_rel = config["workspace"]["historic"]["relative_paths"]["last_run_hash_file"]
            self.__last_commit_hash_path = os.path.join(self.__historic_workspace, last_commit_hash_path_rel)
            self.__repo = Repo(self.__repo_dir)
            self.__branch = self.__repo.current_branch
            self.__dst_commit = dst_commit
            self.__src_commit = None
            self.__has_src_commit = False
            self.__tiaf_bin = config["repo"]["tiaf_bin"]
            if self.__repo.current_branch == self.__source_of_truth:
                self.__is_source_of_truth = True
            else:
                self.__is_source_of_truth = False
            if not os.path.isfile(self.__tiaf_bin):
                raise FileNotFoundError("Could not find tiaf binary")
            self.__generate_change_list()

    def __generate_change_list(self):
        # Determine the change list bewteen now and the last tiaf run (if any)
        self.__has_change_list = False
        self.__change_list_path = None
        if os.path.isfile(self.__last_commit_hash_path):
            with open(self.__last_commit_hash_path) as file:
                self.__src_commit = file.read()
                self.__has_src_commit = True
                if git_utils.is_descendent(self.__src_commit, self.__dst_commit) == False:
                    print(f"Source commit '{self.__src_commit}' and destination commit '{self.__dst_commit}' are not related")
                    return
                diff_path = os.path.join(self.__temp_workspace, "changelist.diff")
                try:
                    git_utils.create_diff_file(self.__ancestor_commit, self.__descendent_commit, diff_path)
                except FileNotFoundError as e:
                    print(e)
                    return
                change_list = {}
                change_list["createdFiles"] = []
                change_list["updatedFiles"] = []
                change_list["deletedFiles"] = []
                with open(diff_path, "r") as diff_data:
                    lines = diff_data.readlines()                        
                    for line in lines:
                        match = re.split("^R[0-9]+\\s(\\S+)\\s(\\S+)", line)
                        if len(match) > 1:
                            # Treat renames as a deletion and an addition
                            change_list["deletedFiles"].append(match[1])
                            change_list["createdFiles"].append(match[2])
                            continue
                        match = re.split("^[AMD]\\s(\\S+)", line)
                        if len(match) > 1:
                            if line[0] == 'A':
                                change_list["createdFiles"].append(match[1])
                            elif line[0] == 'M':
                                change_list["updatedFiles"].append(match[1])
                            elif line[0] == 'D':
                                change_list["deletedFiles"].append(match[1])
                change_list_json = json.dumps(change_list, indent = 4)
                change_list_path = os.path.join(self.__temp_workspace, "changelist.json")
                f = open(change_list_path, "w")
                f.write(change_list_json)
                f.close()
                self.__has_change_list = True
                self.__change_list_path = change_list_path                
        else:
            self.__has_change_list = False
            return

    def dump_config(self):
        print(f"Repository         : {self.__repo_dir}")
        print(f"Branch             : {self.__branch}")
        print(f"Source of Truth    : {self.__source_of_truth}")
        print(f"Is Source of Truth?: {self.__is_source_of_truth}")
        print(f"Has Source Commit? : {self.__has_src_commit}")
        print(f"Has Change List?   : {self.__has_change_list}")
        print(f"Change List Path   : {self.__change_list_path}")

    def run(self, sequence_type, safe_mode):
        args = []
        if sequence_type == SequenceType.REGULAR:
            print("Sequence type: regular")
            args.append("--sequence=regular")
        elif sequence_type == SequenceType.TEST_IMPACT_ANALYSIS:
            print("Sequence type: test impact analysis")
            if self.__is_source_of_truth:
                # Source of truth branch will allways attempt a seed if no test impact analysis data is available
                print("This branch is the source of truth, a seed sequence will be run if there is no test impact analysis data")
                args.append("--sequence=tiaorseed")
            else:
                # Non source of truth branches will fall back to a regular test run if no test impact analysis data is available
                print("This branch is not the source of truth, a regular sequence will be run if there is no test impact analysis data")
                args.append("--sequence=tia")
            
            if safe_mode == True:
                print("Safe mode is on, the deselected test targets will be run after the selected test targets")
                args.append("--safemode=on")
            else:
                print("Safe mode is off, the deselected test targets will be discarded")

            if self.__has_change_list:
                print(f"A change list was generated between the ancestor commit '{self.__ancestor_commit}' and the descendent commit '{self.__descendent_commit}'")
                args.append(f"-changelist={self.__change_list_path}")
            else:
                print(f"No change list was generated, this will cause test impact analysis sequences on branches other than the source of truth to fall back to a regular sequence")

            # probs want to copy over historic data such as last target list etc.
        else:
            raise ValueError(sequence_type)
        result = subprocess.run([self.__tiaf_bin] + args)
        return result.returncode

