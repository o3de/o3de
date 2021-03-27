"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

def extract_log_lines(start_line_marker, end_line_marker, log_file, platform, level):
    """
    Parses log for a start and end marker, and extracts all lines between the markers.
    :param start_line_marker: String that marks beginning of loglines to extract.
    :param end_line_marker: String that marks end of loglines to extract.
    :param log_file: Path to log to parse.
    :param platform: Platform under test passed from test parameterization.
    :param level: Level under test passed from test parameterization.
    """
    extracted_log = '{}_{}_log.txt'.format(platform, level)

    with open(log_file) as log:
        match = False
        new_log = None

        for line in log:
            if start_line_marker in line:
                match = True
                new_log = open(extracted_log, 'w')
            elif end_line_marker in line:
                break
            elif match:
                new_log.write(line)
        if new_log:
            new_log.close()
