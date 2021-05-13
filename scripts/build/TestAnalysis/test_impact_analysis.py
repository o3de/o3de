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

import argparse
import sys
import os
import gitutils
import datetime
import json
import socket

class FilebeatClient(object):
    #def __init__(self, logger, host, port, timeout):
    def __init__(self, host, port, timeout):
        #self._logger = logger.getChild("filebeat_client")
        self._filebeat_host = host
        self._filebeat_port = port
        self._socket_timeout = timeout
        self._socket = None

        self._open_socket()

    def send_event(self, payload, index, timestamp=None, pipeline="filebeat"):
        if timestamp is None:
            timestamp = datetime.datetime.utcnow().timestamp()

        event = {
            "index": index,
            "timestamp": timestamp,
            "pipeline": pipeline,
            "payload": json.dumps(payload)
        }

        # Serialise event, add new line and encode as UTF-8 before sending to Filebeat.
        data = json.dumps(event, sort_keys=True) + "\n"
        data = data.encode()

        #self._logger.debug(f"-> {data}")
        self._send_data(data)

    def _open_socket(self):
        #self._logger.info(f"Connecting to Filebeat on {self._filebeat_host}:{self._filebeat_port}")

        self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._socket.settimeout(self._socket_timeout)

        #try:
        self._socket.connect((self._filebeat_host, self._filebeat_port))
        #except (ConnectionError, socket.timeout):
        #    raise FilebeatExn("Failed to connect to Filebeat") from None

    def _send_data(self, data):
        total_sent = 0

        while total_sent < len(data):
            try:
                sent = self._socket.send(data[total_sent:])
            except BrokenPipeError:
                #self._logger.debug("Filebeat socket closed by peer")
                self._socket.close()
                self._open_socket()
                total_sent = 0
            else:
                total_sent = total_sent + sent

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--currentbranch', dest="currentbranch", help="Name of the current branch", required=True)
    parser.add_argument('-s', '--sourceoftruthbranch', dest="sourceoftruthbranch", help="Name of the source of truth branch to update the test impact analysis data with", required=True)
    parser.add_argument('-l', '--commit', dest="dst_commit", help="Commit to run test impact analysis on (if empty, the most recent commit will be used)")
    parser.add_argument('-b', '--bin', dest="bin", help="Path to the tiaf binary", required=True)
    parser.add_argument('-p', '--persistentdir', dest="persistentdir", help="Path to the tiaf persistent data directory", required=True)
    parser.add_argument('-t', '--tempdir', dest="tempdir", help="Path to the tiaf temp data directory", required=True)
    parser.add_argument('--diffwithhead', dest='diffwithhead', action='store_true', help="If set, will diff the specified commit with the repo head instead of the commit from the last run")
    parser.set_defaults(diffwithhead=False)
    parser.set_defaults(dst_commit="HEAD^")

    args = parser.parse_args()
    
    return args

if __name__ == "__main__":
    args = parse_args()

    if args.diffwithhead is True:
        src_commit = "HEAD"
    else:
        last_commit_hash_path = os.path.join(args.persistentdir, 'last_run.hash')
        if os.path.isfile(last_commit_hash_path):
            print('Found last commit hash file')
            with open(last_commit_hash_path) as file:
                src_commit = file.read()
        else:
            src_commit = None

    output_diff_path = os.path.join(args.tempdir, 'changelist.diff')
    if gitutils.is_descendent(src_commit, args.dst_commit):
        ancestor_commit = src_commit
        descendent_commit = args.dst_commit
    elif gitutils.is_descendent(args.dst_commit, src_commit):
        ancestor_commit = args.dst_commit
        descendent_commit = src_commit
    else:
        print(f"{src_commit} and {args.dst_commit} are not related!")
        sys.exit(1)
    
    print(f"ancestor_commit: {ancestor_commit}")
    print(f"descendent_commit: {descendent_commit}")

    has_changelist = gitutils.create_diff_file(ancestor_commit, descendent_commit, output_diff_path)

    #change_list = {"create": ["file1.cpp", "file2.cpp"], "update": ["file3.cpp", "file4.cpp"], "delete": ["file5.cpp"]}
    #contents = {"branch": "MyBranch", "source_of_truth_branch": "main", "ancestor_commit": "xyx", "descendent_commit": "abc", "unified_diff": "diff contents", "change_list": change_list}

    #file_beat_client = FilebeatClient("127.0.0.1", 9000, 60)
    #file_beat_client.send_event(contents, "jonawals.tiaf.change_list.2021", datetime.datetime.utcnow().timestamp())

    #output_diff_path = os.path.join(args.tempdir, 'changelist.diff')
    #if args.changelist is None:
    #    create_diff_from_last_run("HEAD^", "HEAD", output_diff_path)
    #    if os.path.isfile(output_diff_path):
    #        with open(output_diff_path) as file:
    #            print(file.read())
    #else:
    #    last_commit_hash_path = os.path.join(args.persistentdir, 'last_run.hash')
    #    if os.path.isfile(last_commit_hash_path):
    #        print('Found last commit hash file')
    #        with open(last_commit_hash_path) as file:
    #            last_commit_hash = file.read()
    #        has_changelist = create_diff_from_last_run(last_commit_hash, args.changelist, output_diff_path)
    sys.exit(0)