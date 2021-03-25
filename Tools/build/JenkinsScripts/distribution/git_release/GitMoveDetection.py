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
from P4 import P4
import subprocess
import os
import re


class MoveDetection:
    """
    This class scans two Perforce branches at a specific revision to determine which files have been moved from one
    branch to another. The core logic relies on finding a historical common ancestor of a file split between the two
    specified branches.
    """
    def __init__(self):
        self.p4 = P4()
        self.p4.connect()
        self.parent = dict()
        self.history_data = dict()
        """
        --- Below is a sample structure for intended use of the 'history_data' dictionary.
        --- This dictionary is constructed/populated what we call build_parent_hash().
         history_data:
        {
            "//lyengine/releases/ver01_10":
            {
                roots:
                {
                    [p4_filepath, revision]
                }
                rev_roots:
                {
                    [p4_filepath, revision]
                }
                files:
                {
                    [p4_filepath, revision]
                }
            }
            "//lyengine/releases/ver01_11":
            {
                ...
            }
        }
        """

    @staticmethod
    def branch_pathname_to_inclusive_pathspec(branch_key):
        if branch_key.endswith('/'):
            return branch_key + '...'
        else:
            return branch_key + '/...'

    @staticmethod
    def generate_filelist_hashes(branch_key, branch_cl_revspec):
        """
        Generates iterable listing of files on a branch for use when calculating file ancestor data.
        :param branch_key:
            The branch to scan for files.
        :param branch_cl_revspec:
            Used to generate the file list at the point in time of specified P4 revspec.
            Typically, this value is with a P4 CL number (i.e. @44569).
        :return complete_file_list_hash, file_list_hash:
            Two iterable collections of file hashes - One including deleted files, another excluding deleted files.
        """
        file_list_filename = branch_key.replace('/', '.') + '_files.log'
        if not os.path.exists(file_list_filename):
            file_list_fp = open(file_list_filename, "w+")

            command = f'p4 files {MoveDetection.branch_pathname_to_inclusive_pathspec(branch_key)}@{branch_cl_revspec}'
            print('Performing: ' + command)
            subprocess.check_call(command.split(), stdout=file_list_fp)
            # begin reading from the start
            file_list_fp.seek(0)
        else:
            file_list_fp = open(file_list_filename, 'r')

        complete_file_list_hash = {}  # All files, including deleted ones.
        file_list_hash = {}  # All files, excluding deleted ones.

        for line in file_list_fp:
            filename = re.sub('(#[0-9][0-9]*) - .*', "", line).strip()
            revision = re.sub('.*(#[0-9][0-9]*) - .*', "\\1", line).strip()
            action = re.sub('.*#[0-9]* - ', '', line).strip()
            if not action.startswith('delete change'):
                file_list_hash[(filename, revision)] = True
            complete_file_list_hash[(filename, revision)] = True

        file_list_fp.close()
        return complete_file_list_hash, file_list_hash

    @staticmethod
    def generate_history_data(branch_cl_revspec, branch_key, complete_file_list_hash):
        """
        Calculates historical data to identify ancestor/roots (original name of a file when added) and
        descendants/reverse-roots (all the possible permutations of an original file, whether via copy, branch, or move)

        :param branch_cl_revspec:
            Used to generate the file list at the point in time of specified P4 revspec.
            Typically, this value is with a P4 CL number (i.e. @44569).
        :param branch_key:
            The branch to scan for files.
        :param complete_file_list_hash:
            An iterable collection of the latests files on the branch to compare with.
        :return temp_rev_roots, temp_roots:
            Dictionaries depicting file ancestors and possible descendants for each file.
        """
        file_log_filename = branch_key.replace('/', '.') + '_filelog.log'
        if not os.path.exists(file_log_filename):
            file_log = open(file_log_filename, "w+")
            command = 'p4 filelog -h -s -p {0}@{1}'.format(MoveDetection.branch_pathname_to_inclusive_pathspec(branch_key),
                                                           branch_cl_revspec)
            print('Performing: ' + command)
            subprocess.check_call(command.split(), stdout=file_log)
            # begin reading from the start, immediately after populating the file.
            file_log.seek(0)
        else:
            file_log = open(file_log_filename, 'r')
        '''
        Loop control vars
        '''
        DEFAULT_VALUE = (str(), -1)
        potential_ancestor = DEFAULT_VALUE  # ( filename, revision )
        current_parsed_filename = DEFAULT_VALUE
        current_branch_filename = DEFAULT_VALUE
        temp_roots = dict()  # for calculation purposes
        temp_rev_roots = dict()  # for calculation purposes
        cur_line = 0
        # Begin parsing file log
        for line in file_log:
            if line.startswith('//'):  # Filename
                potential_ancestor = current_parsed_filename
                current_parsed_filename = (line.strip(), -1)

            elif line.startswith('... #'):  # Revision
                if current_parsed_filename[1] == -1:  # If no revision has been found yet...
                    current_parsed_filename = (current_parsed_filename[0], line.split()[1])  # Gets the revision number.

                    # If we are parsing a filename existing in our current/latest revision...
                    # We use the complete file list hash because we want to account for deleted files when
                    # building the ancestry data. Unfortunately, 'p4 filelog' does not support excluding deleted files.
                    # We have to filter this out manually...
                    if current_parsed_filename in complete_file_list_hash:
                        # Treat this filename as a child filename, and begin scanning it's ancestors.
                        # This is the starting point of a file's rename/move history.

                        # If the 'current_branch_filename' IS NOT the default value...
                        # (This basically means we avoid a default-initialization value as a key in the dict.)
                        if current_branch_filename != DEFAULT_VALUE:
                            # Close out history on prior file...

                            # Track filename root
                            temp_roots[current_branch_filename] = potential_ancestor
                            # Track filename reverse root.
                            if potential_ancestor not in temp_rev_roots:
                                temp_rev_roots[potential_ancestor] = list()
                            temp_rev_roots[potential_ancestor].append(current_branch_filename)

                        # Start tracking history of the next file
                        current_branch_filename = current_parsed_filename

            cur_line += 1
        file_log.close()
        # Close history for the last file in the log file's history/entry.
        temp_roots[current_branch_filename] = potential_ancestor
        if potential_ancestor not in temp_rev_roots:
            temp_rev_roots[potential_ancestor] = list()
        temp_rev_roots[potential_ancestor].append(current_branch_filename)
        return temp_rev_roots, temp_roots

    def build_parent_hash(self, branch_key, branch_cl_revspec):
        """
        :param branch_key:
            The branch to scan for files.
        :param branch_cl_revspec:
            Used to generate the file list at the point in time of specified P4 revspec.
            Typically, this value is with a P4 CL number (i.e. @44569).
        """

        # Get files list
        complete_file_list_hash, file_list_hash = self.generate_filelist_hashes(branch_key, branch_cl_revspec)
        # Get file history
        file_reverse_roots, file_roots = self.generate_history_data(branch_cl_revspec, branch_key,
                                                                    complete_file_list_hash)

        # Construct results. Save data to class members.
        self.history_data[branch_key] = dict()
        self.history_data[branch_key]['roots'] = file_roots
        self.history_data[branch_key]['rev_roots'] = file_reverse_roots
        # Below, we save only the currently existing files as a means to iterate over all files, without having to query
        # Perforce continuously.
        self.history_data[branch_key]['files'] = file_list_hash

    def find_moved_files_between_branches(self, p4_branch_name_src, p4_branch_name_dst):
        """
        Find files in revisionB that have moved from revisionA
        """
        file_move = list()

        for Bfile in self.history_data[p4_branch_name_dst]['files']:
            root_b = self.history_data[p4_branch_name_dst]['roots'][Bfile]
            dest_filename = Bfile[0].split(p4_branch_name_dst)[1]

            # If 'Bfile' shares a common ancestor with any file in 'branchA'...
            if root_b in self.history_data[p4_branch_name_src]['rev_roots']:
                reverse_roots_a = self.history_data[p4_branch_name_src]['rev_roots'][root_b]  # Related candidates

                # Scan the candidates to see if any of them depict the file WAS NOT moved/branched/copied.
                found_exact_file_in_both_branches = False
                for Afile in reverse_roots_a:
                    src_filename = Afile[0].split(p4_branch_name_src)[1]
                    if src_filename == dest_filename:
                        found_exact_file_in_both_branches = True
                        break

                # If there is no sign of the file in the other branch, we have moved the file.
                if found_exact_file_in_both_branches is False:
                    # Register a file move
                    file_move.append((src_filename, dest_filename))
                    print(file_move[-1])

        return self.filter_file_moves_to_dev(file_move)

    @staticmethod
    def filter_file_moves_to_dev(file_moves):
        filtered_moves = list()
        for move in file_moves:
            if move[0].startswith('dev/'):
                filtered_moves.append(move)
        return filtered_moves

    @staticmethod
    def chrono_sort_moves(move_list):
        """
        Sorts file moves in chronological operations to avoid out-of-order rename stomping.
        :param move_list:
            List of tuples {src_filename, dst_filename}
        :return:
            A sorted list that can be iterated from beginning to end for rename opterations, without stomping conflicts.
        """
        # Iterate through all the moves to construct a linked list.
        head_to_tail_mapping = dict()  # All the filenames for the start of a chain. (For discovering insertion points)
        chains = dict()  # A collecton of a chain of moves (A->B->C->D file renames)
        tail_to_head_mapping = dict()  # All the filenames at the end of a chain. (For discovering insertion points)

        for move in move_list:  # tuple: (src, dst)
            src = move[0]
            dst = move[1]

            # Create a chain for this move.
            chains[src] = [src, dst]
            head_to_tail_mapping[src] = dst
            tail_to_head_mapping[dst] = src

            # Possible outcomes:
            # Extending the end of an existing chain...
            if src in tail_to_head_mapping:

                # Update our tails & heads
                new_tail = head_to_tail_mapping[src]  # Tail of the chain starting with 'src'
                new_head = tail_to_head_mapping[src]  # Tail of the chain ending with 'src'

                # Join above two chains together.
                tail_to_head_mapping[new_tail] = new_head
                head_to_tail_mapping[new_head] = new_tail

                # Update chain.
                chains[new_head] = chains[new_head] + chains[src][1:]  # Remove first duplicate entry

                # Clean-up
                del head_to_tail_mapping[src]
                del tail_to_head_mapping[src]
                del chains[src]

            # Extending the beginning of an existing chain...
            if dst in head_to_tail_mapping:
                # Update our tails & heads
                new_tail = head_to_tail_mapping[dst]  # Tail of the chain starting with 'dst'
                new_head = tail_to_head_mapping[dst]  # Tail of the chain ending with 'dst'

                # Extend.
                chains[new_head] = chains[new_head] + chains[dst][1:]  # Remove first duplicate entry

                # Join above two chains together.
                tail_to_head_mapping[new_tail] = new_head
                head_to_tail_mapping[new_head] = new_tail

                # Clean-up.
                del head_to_tail_mapping[dst]
                del tail_to_head_mapping[dst]
                del chains[dst]

        # Construct list from chains
        return_list = list()
        for cur_chain in chains:
            previous_filename = None

            reverse_chain = chains[cur_chain]
            reverse_chain.reverse()

            for current_filename in reverse_chain:
                if previous_filename:
                    # We are appending in reverse order.
                    # When renaming, we go from the end of the list, to the beginning.
                    # This way we avoid stomping renames.
                    return_list.append((current_filename, previous_filename))
                previous_filename = current_filename

        return return_list

    def generate_list_files_moved_between_branches(self, branch_cl_tuple_src, branch_cl_tuple_dst):
        """
        :param branch_cl_tuple_src:
            {Tuple} (branch, revision/build number)
        :param branch_cl_tuple_dst:
            {Tuple} (branch, revision/build number)
        :return:
            A list of tuples (filename before, filename after) ordered by intended chronological move operations
        """
        self.build_parent_hash(branch_cl_tuple_src[0], branch_cl_tuple_src[1])
        self.build_parent_hash(branch_cl_tuple_dst[0], branch_cl_tuple_dst[1])
        file_moves = self.find_moved_files_between_branches(branch_cl_tuple_src[0], branch_cl_tuple_dst[0])
        return self.chrono_sort_moves(file_moves)
