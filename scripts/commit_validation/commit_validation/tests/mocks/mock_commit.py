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

from typing import Dict, List

from commit_validation.commit_validation import Commit

class MockCommit(Commit):

    def __init__(self, files=None, removed_files=None, file_diffs=None, description: str = '', author: str = ''):
        self.files = [] if files is None else files
        self.removed_files = [] if removed_files is None else removed_files
        self.file_diffs = {} if file_diffs is None else file_diffs
        self.description = description
        self.author = author

    def get_files(self) -> List[str]:
        return self.files

    def get_removed_files(self) -> List[str]:
        return self.removed_files

    def get_file_diff(self, file) -> Dict[str, str]:
        return  self.file_diffs[file]

    def get_description(self) -> str:
        return self.description

    def get_author(self) -> str:
        return self.author
