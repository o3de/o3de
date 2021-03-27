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
import datetime
import hashlib
import json
import math
import os
import Queue
import re
import shutil
import ssl
import subprocess
import sys
import threading
import time
import urllib2
import urlparse
import zipfile
from collections import deque
from distutils import dir_util, file_util, spawn
from distutils.errors import DistutilsFileError

importDir = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(importDir, ".."))  # Required for AWS_PyTools
from AWS_PyTools import LyChecksum
from GitStaging import get_directory_size_in_bytes, URL_KEY, CHECKSUM_KEY, SIZE_KEY, BOOTSTRAP_CONFIG_FILENAME

FETCH_CHUNK_SIZE = 1000000
CHUNK_FILE_SIZE = 100000000    # 100 million bytes per chunk file
WORKING_DIR_NAME = "_temp"
DOWNLOAD_DIR_NAME = "d"
UNPACK_DIR_NAME = "u"

DOWNLOADER_THREAD_COUNT = 20

# The downloader is intended to be an executable. Typically, executables should have their version bake into the binary.
# Windows has two variations of versions for a binary file: FileVersion, ProductVersion. Furthermore, we need to import
# Windows-specific apis to read Windows executable binary versions. Other operating systems have their own versioning scheme.
# To simplify maintenance, we will simply track the version in the source code itself.
DOWNLOADER_RELEASE_VERSION = "1.2"

FILESIZE_UNIT_ONE_INCREMENT = 1024
FILESIZE_UNIT_TWO_INCREMENT = FILESIZE_UNIT_ONE_INCREMENT * FILESIZE_UNIT_ONE_INCREMENT
FILESIZE_UNIT_THREE_INCREMENT = FILESIZE_UNIT_TWO_INCREMENT * FILESIZE_UNIT_ONE_INCREMENT

HASH_FILE_NAME = "filehashes.json"
DEFAULT_HASH_FILE_URL = "https://d3dn1rjl3s1m7l.cloudfront.net/default-hash-file/" + HASH_FILE_NAME

TRY_AGAIN_STRING = "Please try again or contact Lumberyard support if you continue to experience issues."

#returns the size of the file moved
def safe_file_copy(src_basepath, dst_basepath, cur_file):
    src_file_path = os.path.join(src_basepath, cur_file)
    dst_file_path = os.path.join(dst_basepath, cur_file)

    dir_util.mkpath(os.path.dirname(dst_file_path))
    dst_name, copied = file_util.copy_file(src_file_path, dst_file_path, verbose=0)

    if copied is False:
        raise Exception("Failed to copy {} to {}.".format(src_file_path, dst_file_path))

    return os.path.getsize(dst_name)


def _get_input_replace_file(filename):

    def _print_invalid_input_given(response):
        print 'Your response of "{0}" is not a valid response. Please enter one of the options mentioned.\n'

    valid_replace_responses = ['y', 'yes', 'yes all']
    valid_keep_responses = ['n', 'no', 'no all']

    response_given = None
    while not response_given:
        print 'A new version of {0} has been downloaded, but a change to the local file has been detected.'.format(filename)
        print 'Would you like to replace the file on disk with the new version? ({0})'.format("/".join(valid_replace_responses + valid_keep_responses))
        print 'Answering "n" will keep the local file with your modificaitions.'
        print 'Ansering "yes all"/"no all" will assume this answer for all subsequent prompts.'
        response = raw_input("Replace the file on disk with the new version?  ({0}) ".format("/".join(valid_replace_responses + valid_keep_responses)))
        print ""
        normalized_input = None
        try:
            normalized_input = response.lower()
            if normalized_input not in valid_replace_responses and \
                normalized_input not in valid_keep_responses:
                _print_invalid_input_given(response)
            else:
                valid_respose = True
                response_given = normalized_input
        except Exception:
            _print_invalid_input_given(response)

    # we know this is valid input. if it is not a replace response, then it must be a keep
    return response_given in valid_replace_responses, 'a' in response_given


def find_files_to_prompt(args, changed_files, dst_basepath, old_file_hashes):
    num_files_to_prompt = 0
    for key in changed_files:
        # get path to file that is currently on disk
        existing_file_path = os.path.join(dst_basepath, key)
        if os.path.exists(existing_file_path):
            # get the hash of the file on disk
            file_hash = LyChecksum.getChecksumForSingleFile(existing_file_path, 'rU').hexdigest()
            # if disk is same as old, replace
            if file_hash == old_file_hashes[key]:
                continue
            # otherwise, ask if keep, replace
            else:
                # assume an answer
                if not (args.yes or args.no):
                    num_files_to_prompt += 1

    return num_files_to_prompt


def partition_moves_and_skips(args, changed_files, dst_basepath, old_file_hashes):
    changed_files_to_move = set()
    changed_files_to_skip = set()

    for key in changed_files:
        should_move_file = False
        # get path to file that is currently on disk
        existing_file_path = os.path.join(dst_basepath, key)
        if os.path.exists(existing_file_path):
            # get the hash of the file on disk
            file_hash = LyChecksum.getChecksumForSingleFile(existing_file_path, 'rU').hexdigest()
            # if disk is same as old, replace
            if file_hash == old_file_hashes[key]:
                should_move_file = True
            # otherwise, ask if keep, replace
            else:
                # assume the answer is to replace
                if args.yes:
                    should_move_file = True
                # assume the answer is to keep
                elif args.no:
                    should_move_file = False
                else:
                    should_move_file, use_as_assumption = _get_input_replace_file(existing_file_path)
                    if use_as_assumption and should_move_file:
                        args.yes = True
                        print "Marking all subsequent files as files to replace."
                    elif use_as_assumption and not should_move_file:
                        args.no = True
                        print "Marking all subsequent files as files to keep."

        # it was deleted on disk, so it should be safe to move over
        else:
            should_move_file = True

        if should_move_file:
            changed_files_to_move.add(key)
        else:
            changed_files_to_skip.add(key)

    return changed_files_to_move, changed_files_to_skip


def load_hashlist_from_json(path):
    file_path = os.path.join(path, HASH_FILE_NAME)
    hash_list = {}
    if not os.path.exists(file_path):
        raise Exception("No hashfile exists at {0}.".format(file_path))
    with open(file_path, 'rU') as hashfile:
        hash_list = json.load(hashfile)
    return hash_list


def copy_directory_contents(args, src_basepath, dst_basepath, uncompressed_size):
    # read in new hashlist
    new_file_hashes = load_hashlist_from_json(src_basepath)

    # read in old hashlist. We check to make sure it is still on disk before we get here.
    old_file_hashes = load_hashlist_from_json(dst_basepath)

    num_files_in_new = len(new_file_hashes.keys())
    print "There are {0} files in the new zip file.\n".format(num_files_in_new)

    old_file_hashes_keys = set(old_file_hashes.keys())
    new_file_hashes_keys = set(new_file_hashes.keys())

    changed_files = old_file_hashes_keys & new_file_hashes_keys # '&' operator finds intersection between sets
    deleted_files = set()
    added_files = set()
    missing_files = set()
    identical_hashes = set()
    changed_files_to_move = set()
    changed_files_to_skip = set()

    identical_files_size_total = 0

    # lets get rid of files that have the same hash, as we dont care about then
    #   skip if the same
    for key in changed_files:
        # if the file doesn't exist on disk, treat it as an add, regardless of whether the filelists have diff hashes
        if not os.path.exists(os.path.join(dst_basepath, key)):
            missing_files.add(key)
        # if the file is on disk, and the hashes in the filelists are the same, there is no action to take, sorecord the progress
        elif old_file_hashes[key] == new_file_hashes[key]:
            identical_files_size_total += os.path.getsize(os.path.join(src_basepath, key))
            del old_file_hashes[key]
            del new_file_hashes[key]
            identical_hashes.add(key)

    # now that we cleared all of the identical hashes, if a file doesn't
    #   exist in the intersection, it is an add or delete, depending on
    #   the source hash list
    deleted_files = old_file_hashes_keys.difference(changed_files)
    added_files = missing_files.union(new_file_hashes_keys.difference(changed_files))

    # cant remove from the set being iterated over, so get the difference between
    #   identical hashes and changed hashes and save it back to the changed set
    changed_files = changed_files.difference(identical_hashes.union(missing_files))

    total_keys = len(old_file_hashes_keys | new_file_hashes_keys)
    keys_across_all_sets = len(changed_files | deleted_files | added_files | missing_files | identical_hashes)
    if total_keys != keys_across_all_sets:
        raise Exception("Not all keys caught in the resulting sets.")

    print "Finding files with conflicts."
    # figure out how many files there are to prompt about
    num_files_to_prompt = find_files_to_prompt(args, changed_files, dst_basepath, old_file_hashes)
    print "There are {0} files with conflicts that need to be asked about.\n".format(num_files_to_prompt)

    # split the files into moves and skips, and ask customers about files with any conflicts
    changed_files_to_move, changed_files_to_skip = partition_moves_and_skips(args, changed_files, dst_basepath, old_file_hashes)


    # find the total size for all the skipped files
    skipped_files_size_total = 0
    for key in changed_files_to_skip:
        skipped_files_size_total += os.path.getsize(os.path.join(src_basepath, key))

    move_progress_meter = ProgressMeter()
    move_progress_meter.action_label = "Moving"
    move_progress_meter.target = float(uncompressed_size)
    move_progress_meter.report_eta = False
    move_progress_meter.report_speed = False
    move_progress_meter.start()

    # initialize the meter with the size of the files not being moved either due to being skipped, or being identical
    move_progress_meter.record_progress(identical_files_size_total + skipped_files_size_total)

    # if in new but not old, keep - it was added
    # also move files that were changed that should be moved
    num_files_moved = 0
    for key in added_files.union(changed_files_to_move):
        dest_file_size = safe_file_copy(src_basepath, dst_basepath, key)
        move_progress_meter.record_progress(dest_file_size)
        num_files_moved += 1

    #   if in old but not new, it was deleted. compare against disk
    num_files_deleted = 0
    for key in deleted_files:
        # get path to file that is currently on disk
        existing_file_path = os.path.join(dst_basepath, key)
        if os.path.exists(existing_file_path):
            # get the hash of the file on disk
            file_hash = LyChecksum.getChecksumForSingleFile(existing_file_path, 'rU').hexdigest()
            # if disk is same as old, deleted, otherwise, we keep the file that is there.
            # not tracked against the progress, as removes are not counted
            #   against the total (the uncompressed size of the zip)
            if file_hash == old_file_hashes[key]:
                os.remove(existing_file_path)
                num_files_deleted += 1

    # move new hashfile over
    dest_file_size = safe_file_copy(src_basepath, dst_basepath, HASH_FILE_NAME)
    move_progress_meter.record_progress(dest_file_size)

    move_progress_meter.stop()

    print "{0}/{1} new files were moved".format(num_files_moved, num_files_in_new)
    print "{0}/{1} files were removed".format(num_files_deleted, len(deleted_files))

def get_default_hashlist(args, dst_basepath, working_dir_path):
    #   acquire default hashlist
    default_hashlist_url = DEFAULT_HASH_FILE_URL
    if args.overrideDefaultHashfileURL is not None:
         default_hashlist_url = args.overrideDefaultHashfileURL

    with Downloader(DOWNLOADER_THREAD_COUNT) as downloader:
        dest = os.path.join(working_dir_path, HASH_FILE_NAME)
        print "Downloading files from url {0} to {1}"\
            .format(default_hashlist_url, dest)
        try:
            files = downloader.download_file(default_hashlist_url, dest, 0, True, True)
        finally:
            downloader.close()
    if not files:
        raise Exception("Failed to finish downloading {0} after a few retries."
                        .format(HASH_FILE_NAME))
    # now that we have the hashlist, move it to the root of the local repo
    safe_file_copy(working_dir_path, dst_basepath, HASH_FILE_NAME)
    os.remove(dest)


def is_url(potential_url):
    return potential_url.startswith('https')


def create_ssl_context():
    ciphers_to_remove = ["RC4", "DES", "PSK", "MD5", "IDEA", "SRP", "DH", "DSS", "SEED", "3DES"]
    cipher_string = ssl._DEFAULT_CIPHERS + ":"
    for idx in range(len(ciphers_to_remove)):
        # create the cipher string to permanently remove all of these ciphers,
        #   based on the format documented at
        #   https://www.openssl.org/docs/man1.0.2/apps/ciphers.html
        cipher_string += "!{}".format(ciphers_to_remove[idx])
        if idx < len(ciphers_to_remove) - 1:
            cipher_string += ":"    # ":" is the delimiter

    ssl_context = ssl.create_default_context(ssl.Purpose.SERVER_AUTH)
    ssl_context.set_ciphers(cipher_string)

    ssl_context.verify_mode = ssl.CERT_REQUIRED
    # I can't find a way to load CRL
    # ssl_context.verify_flags = ssl.VERIFY_CRL_CHECK_CHAIN

    return ssl_context


#
# Disk space
#
def get_free_disk_space(dir_name):
    # Get the remaining space on the drive that the given directory is on
    import platform
    import ctypes
    if platform.system() == 'Windows':
        free_bytes = ctypes.c_ulonglong(0)
        ctypes.windll.kernel32.GetDiskFreeSpaceExW(ctypes.c_wchar_p(dir_name), None, None, ctypes.pointer(free_bytes))
        return free_bytes.value
    else:
        st = os.statvfs(dir_name)
        return st.f_bavail * st.f_frsize


#
# Checksum
#
def get_checksum_for_multi_file(multi_file):
    block_size = 65536
    fileset_hash = hashlib.sha512()
    buf = multi_file.read(block_size)
    while len(buf) > 0:
        fileset_hash.update(buf)
        buf = multi_file.read(block_size)
    return fileset_hash


def get_zip_info_from_json(zip_descriptor):
    try:
        url = zip_descriptor[URL_KEY]

        checksum = zip_descriptor[CHECKSUM_KEY]
        if not LyChecksum.is_valid_hash_sha512(checksum):
            raise Exception("The checksum found in the config file is not a valid SHA512 checksum.")

        size = zip_descriptor[SIZE_KEY]
        if not size > 0:
            raise Exception("The uncompressed size mentioned in the config file is "
                            "a value less than, or equal to zero.")
    except KeyError as missingKey:
        print "There is a key, value pair missing from the bootstrap configuration file."
        print "Error: {0}".format(missingKey)
        raise missingKey
    except Exception:
        raise
    return url, checksum, size


def get_info_from_bootstrap_config(config_filepath):
    zip_descriptor = {}
    if not os.path.exists(config_filepath):
        raise Exception("Could not find bootstrap config file at the root of the repository ({0}). "
                        "Please sync this file from the repository again."
                        .format(bootstrap_config_file))
    with open(config_filepath, 'rU') as config_file:
        zip_descriptor = json.load(config_file)
    try:
        url, checksum, size = get_zip_info_from_json(zip_descriptor)
    except Exception:
        raise

    return url, checksum, size


#
# Args
#
def create_args():
    parser = argparse.ArgumentParser(description="Downloads required files relevant to the repositiry HEAD "
                                                 "to complete Lumberyard setup via Git.")
    parser.add_argument('--rootDir',
                        default=os.path.dirname(os.path.abspath(__file__)),
                        help="The location of the root of the repository.")
    parser.add_argument('--pathToGit',
                        default=spawn.find_executable("git"),
                        help="The location of the git executable. Git is assumed to be in your path if "
                             "this argument is not provided.")
    parser.add_argument('-k', '--keep',
                        default=False,
                        action='store_true',
                        help='Keep downloaded files around after download finishes. (default False)')
    parser.add_argument('-c', '--clean',
                        default=False,
                        action='store_true',
                        help='Remove any temp files before proceeding. (default False)')
    parser.add_argument('-v', '--verbose',
                        default=False,
                        action='store_true',
                        help='Enables logging messages. (default False)')
    parser.add_argument('--version',
                        default=False,
                        action='store_true',
                        help='Print application version')
    parser.add_argument('-s', '--skipWarning',
                        default=False,
                        action='store_true',
                        help='Skip all warnings produced. (default False)')
    # If specified, download the hashfile from the given location
    parser.add_argument('--overrideDefaultHashfileURL',
                        default=None,
                        help=argparse.SUPPRESS)
    group = parser.add_mutually_exclusive_group()
    group.add_argument('-y', "--yes",
                        default=False,
                        action='store_true',
                        help='Will automatically answer "yes" to all files being asked to be overwritten. Only specify one of either --yes or --no. (default False)')
    group.add_argument('-n', "--no",
                        default=False,
                        action='store_true',
                        help='Will automatically answer "no" to all files being asked to be overwritten. Only specify one of either --yes or --no. (default False)')

    args, unknown = parser.parse_known_args()
    return args


def validate_args(args):
    if args.version:
        print DOWNLOADER_RELEASE_VERSION
        sys.exit(0)

    assert (os.path.exists(args.rootDir)), "The root directory specified (%r) does not exist." % args.rootDir

    # check to make sure git exists either from the path or user specified location
    if args.pathToGit is None:
        raise Exception("Cannot find Git in your environment path. This scripts requires Git to be installed.")
    else:
        if os.path.isfile(args.pathToGit) is False:
            raise Exception("The path to Git provided does not exists.")


class ProgressMeter:
    def __init__(self):
        self.event = None
        self.worker = None

        self.lock = threading.Lock()
        self.startTime = 0
        self.rateSamples = deque()

        self.action_label = ""
        self.target = 0
        self.progress = 0

        self.report_eta = True
        self.report_speed = True
        self.report_target = True

        self.report_bar = True
        self.report_bar_width = 10

        self.prev_line_length = 0

        self.spinner_frames = ["|", "/", "-", "\\"]
        self.curr_spinner_frame = 0

    @staticmethod
    def meter_worker(meter, event):
        while not event.is_set():
            try:
                meter.report_progress()
                time.sleep(0.25)
            except Exception:
                pass

    def add_target(self, i):
        self.lock.acquire()
        try:
            self.target += i
        finally:
            self.lock.release()

    def record_progress(self, i):
        self.lock.acquire()
        try:
            self.progress += i
        finally:
            self.lock.release()

    def reset(self):
        self.startTime = 0
        self.rateSamples = deque()

        self.action_label = ""
        self.target = 0
        self.progress = 0

        self.report_eta = True
        self.report_speed = True
        self.report_target = True

        self.report_bar = True
        self.report_bar_width = 10

        self.prev_line_length = 0
        self.curr_spinner_frame = 0

    def start(self):
        self.event = threading.Event()
        self.worker = threading.Thread(target=self.meter_worker, args=(self, self.event))
        self.worker.setDaemon(True)
        self.worker.start()
        self.startTime = time.clock()

    # Set up so we can work with with statement, and auto destruct
    def __enter__(self):
        self.start()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        # Make sure the thread stops
        self.stop()

    def __del__(self):
        if self.event:
            self.event.set()

    def stop(self):
        self.report_progress()        # Final progress report, to show completion
        self.event.set()
        print ""                      # Set a new line from all other print operations

    def build_report_str(self, percent_complete, rate, eta):
        # Build output report string
        output_str = "{}".format(self.action_label)

        if self.report_target is True:
            output_str += " {:4.2f} GB".format(float(self.target) / FILESIZE_UNIT_THREE_INCREMENT)

        if self.report_speed is True:
            output_str += " @ {:5.2f} MB/s".format(rate / FILESIZE_UNIT_TWO_INCREMENT)

        if self.report_bar is True:
            percent_per_width = 100.0 / self.report_bar_width
            current_bar_width = percent_complete * 100.0 / percent_per_width
            current_bar_width = int(math.floor(current_bar_width))
            remaining_width = self.report_bar_width - current_bar_width

            curr_spinner_icon = ""
            if remaining_width is not 0:
                curr_spinner_icon = self.spinner_frames[self.curr_spinner_frame]

            output_str += " [" + ("=" * current_bar_width) + curr_spinner_icon + (" " * (remaining_width - 1)) + "]"

        output_str += " {:.0%} complete.".format(percent_complete)

        if self.report_eta is True:
            output_str += " ETA {}.".format(str(datetime.timedelta(seconds=eta)))

        return output_str

    def report_progress(self):
        self.lock.acquire()
        try:
            if self.target == 0:
                percent_complete = 1.0
            else:
                percent_complete = self.progress * 1.0 / self.target

            self.rateSamples.append([self.progress, time.clock()])
            # We only keep 40 samples, about 10 seconds worth
            if len(self.rateSamples) > 40:
                self.rateSamples.popleft()
            if len(self.rateSamples) < 2:
                rate = 0.0
            else:
                # Calculate rate from oldest sample and newest sample.
                span = float(self.rateSamples[-1][0] - self.rateSamples[0][0])
                duration = self.rateSamples[-1][1] - self.rateSamples[0][1]
                rate = span / duration

            if percent_complete == 1.0:
                eta = 0
            elif rate == 0.0:
                eta = 100000
            else:
                eta = int((self.target - self.progress) / rate)

            self.curr_spinner_frame = (self.curr_spinner_frame + 1) % len(self.spinner_frames)
            output_str = self.build_report_str(percent_complete, rate, eta)

            # Calculate the delta of prev and curr line length to clear
            curr_line_length = len(output_str)
            line_len_delta = max(self.prev_line_length - curr_line_length, 0)

            # Extra spaces added to the end of the string to clear the unused buffer of previous write
            sys.stdout.write("\r" + output_str + " " * line_len_delta) # \r placed at the beginning to play nice with PyCharm.
            sys.stdout.flush()
            self.prev_line_length = curr_line_length

        except Exception as e:
            print "Exception: ", e
            sys.stdout.flush()
        finally:
            self.lock.release()


class Downloader:
    meter = ProgressMeter()
    download_queue = Queue.Queue()
    max_worker_threads = 1
    max_retries = 3
    timeout = 5     # in seconds.
    event = None

    def __init__(self, max_threads=1, max_retries=3):
        self.max_worker_threads = max_threads
        self.retries = max_retries
        self.event = threading.Event()

        # preallocate the worker threads.
        for i in range(self.max_worker_threads):
            worker = threading.Thread(target=self.download_chunk_file, args=(self.download_queue, self.event))
            worker.daemon = True
            worker.start()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        # Make sure the threads stop
        self.event.set()

    def __del__(self):
        self.event.set()

    def close(self):
        self.event.set()

    def download_chunk(self):
        pass

    def download_chunk_segments(self, start, end, file_path, url, exit_event):
        # Set up so that we can resume a download that was interrupted
        try:
            existing_size = os.path.getsize(file_path)
        except os.error as e:
            "Exception: {}".format(e)
            existing_size = 0

        # offset by the size of the already downloaded file so we can resume
        start = start + existing_size

        # if the existing size of the file matches the expected size, then we already have the file, so skip it.
        if existing_size is not min((end - start)+1, CHUNK_FILE_SIZE):
            segments = int(math.ceil(float((end-start)+1)/float(FETCH_CHUNK_SIZE)))

            with open(file_path, 'ab') as chunk_file:
                for segment in range(segments):
                    # check for the exit event
                    if exit_event.is_set():
                        break

                    segment_start = start + (segment * FETCH_CHUNK_SIZE)
                    segment_end = min(end, (segment_start + FETCH_CHUNK_SIZE) - 1)
                    byte_range = '{}-{}'.format(segment_start, segment_end)
                    chunk_content_read_size = 10000
                    try:
                        request_result = urllib2.urlopen(
                            urllib2.Request(url, headers={'Range': 'bytes=%s' % byte_range}), timeout=self.timeout)
                        # Result codes 206 and 200 are both considered successes
                        if not (request_result.getcode() == 206 or request_result.getcode() == 200):
                            raise Exception("URL Request did not succeed. Error code: {}"
                                            .format(request_result.getcode()))
                        while True:
                            data = request_result.read(chunk_content_read_size)
                            if exit_event.is_set() or not data:
                                break
                            self.meter.record_progress(len(data))
                            chunk_file.write(data)
                            chunk_file.flush()
                    except Exception:
                        raise

    # Helper thread worker for Downloader class
    def download_chunk_file(self, queue, exit_event):
        while not exit_event.is_set():
            try:
                job = queue.get(timeout=1)
                try:
                    start = job['start']
                    end = job['end']
                    file_path = job['file']
                    url = job['url']
                    for i in range(self.max_retries):
                        if exit_event.is_set():
                            break
                        try:
                            self.download_chunk_segments(start, end, file_path, url, exit_event)
                        except Exception:
                            # if the try throws, we retry, so ignore
                            pass
                        else:
                            break
                    else:
                        raise Exception("GET Request for {} failed after retries. Site down or network disconnected?"
                                        .format(file_path))
                finally:
                    queue.task_done()
            except Exception:
                # No jobs in the queue. Don't error, but don't block on it. Otherwise,
                # the daemon thread cant quit when the event was set
                pass

    def simple_download(self, url, dest):
        self.meter.reset()
        self.meter.action_label = "Downloading"
        self.meter.start()
        request_result = urllib2.urlopen(urllib2.Request(url), timeout=self.timeout)
        if request_result.getcode() != 200:
            raise ValueError('HEAD Request failed.', request_result.getcode())
        with open(dest, 'wb') as download_file:
            data = request_result.read()
            if data:
                self.meter.record_progress(len(data))
                download_file.write(data)
                download_file.flush()
        self.meter.stop()
        self.meter.reset()

    def download_file(self, url, dest, expected_uncompressed_size, force_simple=False, suppress_suffix=False):
        start_time = time.clock()

        # ssl tests
        ssl_context = create_ssl_context()
        for i in range(self.max_retries):
            try:
                request_result = urllib2.urlopen(urllib2.Request(url), timeout=10, context=ssl_context)
                # should not hard code this... pass this to an error handling function to figure out what to do
                if request_result.getcode() != 200:
                    raise ValueError('HEAD Request failed.', request_result.getcode())

            except ssl.SSLError as ssl_error:
                raise Exception("SSL ERROR: Type: {0}, Library: {1}, Reason: {2}."
                                .format(type(ssl_error), ssl_error.library, ssl_error.reason))

            except ssl.CertificateError:
                raise

            except urllib2.HTTPError:
                raise

            except urllib2.URLError as e:
                if isinstance(e.reason, ssl.SSLError):
                    # raise the SSLError exception we encountered and stop downloading
                    raise e.reason
                pass    # we'll ignore the other URLErrors for now, it'll be caught in the else statement below

            except Exception as e:
                import traceback
                print "Generic exception caught: " + traceback.format_exc()
                print str(e)
                pass    # we'll ignore the error now. Might want to put this into a "most recent error" var for later

            else:
                break   # we got the result, so no need to loop further"""

        else:
            # we went through the loop without getting a result. figure out what the errors were and report it upwards
            raise Exception('HEAD Request failed after retries. Site down or network disconnected?')

        file_size = int(request_result.headers.getheader('content-length'))
        # check disk to see if there is enough space for the compressed file and the uncompressed file
        remaining_disk_space = get_free_disk_space(os.path.dirname(dest))
        operation_required_size = file_size + expected_uncompressed_size
        if operation_required_size > remaining_disk_space:
            raise Exception("There is not enough space on disk ({}) to perform the operation. "
                            "Please make sure that {}GB of free space is available then try again."
                            .format(dest, operation_required_size
                                    / FILESIZE_UNIT_THREE_INCREMENT))

        # We may be re-running the script from a previous attempt where we have already partially downloaded some files.
        # Calculate the actual amount to be downloaded.
        dest_directory = os.path.dirname(os.path.abspath(dest))
        dest_byte_size = get_directory_size_in_bytes(os.path.abspath(dest_directory))
        self.meter.add_target(file_size)
        self.meter.record_progress(dest_byte_size)

        ranges_available = request_result.headers.getheader('accept-ranges')
        if ranges_available != 'bytes' or force_simple is True:
            # download without using ranges
            download_dest = dest
            if not suppress_suffix:
                download_dest += ".000"
            self.simple_download(url, download_dest)
            return download_dest
        else:
            # We have byte ranges, so we can download in chunks in
            # parallel. We download into multiple files, which we
            # will recombine with the file inputs function to pass
            # into the unzip function later.
            # This allows a clean resume with parallel gets from
            # different parts of the overall range.

            chunk_files = int(math.ceil(float(file_size) / float(CHUNK_FILE_SIZE)))
            # break into a collection of <chunkFiles> files
            file_list = ["{}.{:04d}".format(dest, x) for x in range(chunk_files)]
            files = [{'start': x * CHUNK_FILE_SIZE,
                      'end': min(((x+1) * CHUNK_FILE_SIZE) - 1, file_size - 1),
                      'file': "{}.{:04d}".format(dest, x),
                      'url': url} for x in range(chunk_files)]

            for entry in files:
                self.download_queue.put(entry)

            self.meter.action_label = "Downloading"
            self.meter.start()

            while self.download_queue.unfinished_tasks:
                time.sleep(0.1)

            # double check all tasks are completed
            self.download_queue.join()

            self.meter.stop()

            if self.meter.progress < self.meter.target:
                print_str = "Download failed. Check network and retry. Elapsed time {}"\
                    .format(str(datetime.timedelta(seconds=time.clock() - start_time)))
                return_list = []
            else:
                print_str = "Finished. Elapsed time {}"\
                    .format(str(datetime.timedelta(seconds=time.clock()-start_time)))
                return_list = file_list

            print print_str
            sys.stdout.flush()
            return return_list


# Class to treat a collection of chunk files as a single larger file.
# We use this to unzip the chunk files as a single file.
# This is a minimal implementation, as required by the zipFile handle.
# This essentially supports only seeking and reading.
# Minimal error processing is present here. Probably needs some more
# to deal with ill formed input files. Right now we just assume
# errors thrown by the underlying system will be the right ones.
class MultiFile:
    fileList = []
    fileSizes = []
    fileOffsets = []
    fileSize = 0
    current_file = 0

    def __init__(self, files, mode):
        self.fileList = files
        self.mode = mode
        for f in files:
            self.fileSizes.append(os.path.getsize(f))
            self.fileOffsets.append(self.fileSize)
            self.fileSize += self.fileSizes[-1]
        try:
            self.cfp = open(self.fileList[0], self.mode)
        except Exception:
            raise

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        pass

    def seek(self, offset, w=0):
        cursor = self.tell()
        if w == os.SEEK_SET:
            cursor = offset
        elif w == os.SEEK_CUR:
            cursor += offset
        elif w == 2:
            cursor = self.fileSize + offset

        # Determine which file we are in now, and do the local seek
        current_pos = cursor
        local_curr_file = 0
        for i in range(len(self.fileSizes) - 1):
            if current_pos < self.fileSizes[i]:
                local_curr_file = i
                break
            current_pos -= self.fileSizes[i]
        else:
            local_curr_file = len(self.fileSizes) - 1

        if self.current_file != local_curr_file:
            self.current_file = local_curr_file
            self.cfp.close()
            self.cfp = open(self.fileList[self.current_file], self.mode)
        self.cfp.seek(current_pos, 0)

    def close(self):
        self.cfp.close()

    def tell(self):
        cpos = self.cfp.tell()
        cursor = self.fileOffsets[self.current_file] + cpos
        return cursor

    def read(self, size=None):
        if size is None:
            size = self.fileSize - self.tell()
        block = self.cfp.read(size)
        remaining = size-len(block)
        # Keep reading if there is size to read remaining, and we are
        # not yet already reading the last file (and may have gotten EOF)
        while remaining > 0 and self.current_file < len(self.fileList)-1:
            # Switch to next file
            self.cfp.close()
            self.current_file += 1
            self.cfp = open(self.fileList[self.current_file], self.mode)
            nblock = self.cfp.read(remaining)
            block += nblock
            remaining -= len(nblock)
        return block


def main():
    try:
        args = create_args()
        validate_args(args)
        abs_root_dir = os.path.abspath(args.rootDir)
        remove_downloaded_files = False
        script_succeed = False

        if args.skipWarning is False:
            print "Now completing your Lumberyard setup."
            print "This downloads essential content not included in the Git repository."
            print "If you've made any changes, please back them up before running this."
            print "Press Enter to continue (Ctrl+C to cancel at anytime)..."
            sys.stdout.flush()

            # blocks until user presses the Enter key
            raw_input()

        # As of 1.8, the longest file path relative to root of the zip is 151,
        # giving 104 chars before the windows path limit. Set the max working dir
        # length to 60 to have some wiggle room.
        max_working_dir_len = 60
        # working dir should be rootDir/working_dir_name. if that is too long, try
        #   using %TEMP%/working_dir_name
        working_dir_path = os.path.join(abs_root_dir, WORKING_DIR_NAME)
        if len(working_dir_path) > max_working_dir_len:
            # switch to using default temp dir
            working_dir_path = os.path.join(os.path.expandvars("%TEMP%"), WORKING_DIR_NAME)
        unpack_dir_path = os.path.join(working_dir_path, UNPACK_DIR_NAME)

        # Remove any pre-downloaded files, if necessary.
        if args.clean and os.path.exists(working_dir_path):
            shutil.rmtree(working_dir_path)

        if not os.path.exists(working_dir_path):
            os.makedirs(working_dir_path)

        # check for old hashlist
        old_hash_file_path = os.path.join(abs_root_dir, HASH_FILE_NAME)
        if not os.path.exists(old_hash_file_path):
            get_default_hashlist(args, abs_root_dir, working_dir_path)

        try:
            try:
                bootstrap_config_file = os.path.join(abs_root_dir, BOOTSTRAP_CONFIG_FILENAME)
                download_url, expected_checksum, uncompressed_size = get_info_from_bootstrap_config(bootstrap_config_file)
                download_file_name = os.path.basename(urlparse.urlparse(download_url)[2])
            except Exception:
                raise

            # check remaining disk space of destination against the uncompressed size
            remaining_disk_space = get_free_disk_space(abs_root_dir)
            if not uncompressed_size < remaining_disk_space:
                raise Exception("There is not enough space on disk ({}) for the extra files. "
                                "Please make sure that {}GB of free space is available then try again."
                                .format(abs_root_dir, uncompressed_size / FILESIZE_UNIT_THREE_INCREMENT))

            # now check against the disk where we are doing the work
            remaining_disk_space = get_free_disk_space(working_dir_path)
            if not uncompressed_size < remaining_disk_space:
                raise Exception("There is not enough space on disk ({}) to perform the operation. "
                                "Please make sure that {}GB of free space is available then try again."
                                .format(working_dir_path, uncompressed_size / FILESIZE_UNIT_THREE_INCREMENT))

            # download the file, with 20 threads!
            try:
                with Downloader(DOWNLOADER_THREAD_COUNT) as downloader:
                    download_dir_path = os.path.join(working_dir_path, DOWNLOAD_DIR_NAME)
                    if not os.path.exists(download_dir_path):
                        os.mkdir(download_dir_path)
                    dest = os.path.join(download_dir_path, download_file_name)

                    print "Downloading files from url {0} to {1}"\
                        .format(download_url, dest)
                    files = downloader.download_file(download_url, dest, uncompressed_size)
            except Exception:
                downloader.close()
                raise

            # if the download failed...
            if not files:
                raise Exception("Failed to finish downloading {0} after a few retries."
                                .format(download_file_name))

            # make the downloaded parts a single file
            multi_file_zip = MultiFile(files, 'rb')

            # check downloaded file against checksum
            print "Checking downloaded contents' checksum."
            downloaded_file_checksum = get_checksum_for_multi_file(multi_file_zip)
            readable_checksum = downloaded_file_checksum.hexdigest()
            if readable_checksum != expected_checksum:
                remove_downloaded_files = True
                raise Exception("The checksum of the downloaded file does not match the expected checksum. ")

            # check if unpack directory exists. clear it if it does.
            delete_existing_attempts = 0
            delete_success = False
            delete_attempts_max = 3
            if os.path.exists(unpack_dir_path):
                while not delete_success and delete_existing_attempts < delete_attempts_max:
                    try:
                        shutil.rmtree(unpack_dir_path)
                    except (shutil.Error, WindowsError, DistutilsFileError) as removeError:
                        delete_existing_attempts += 1
                        if delete_existing_attempts >= delete_attempts_max:
                            raise removeError
                        print ("{0}: {1}").format(type(removeError).__name__, removeError)
                        print ("Failed to remove files that already existed at {} before unpacking. Please ensure the files"
                               " are deletable by closing related applications (such as Asset Processor, "
                               "and the Lumberyard Editor), then try running this program again.").format(unpack_dir_path)
                        raw_input("Press ENTER to retry...")
                    except Exception:
                        raise
                    else:
                        delete_success = True
            os.mkdir(unpack_dir_path)

            # unpack file to temp directory.
            zip_file = zipfile.ZipFile(multi_file_zip, allowZip64=True)
            try:
                print "Extracting all files from {0} to {1}".format(download_file_name, unpack_dir_path)

                extract_progress_meter = ProgressMeter()
                extract_progress_meter.action_label = "Extracting"
                extract_progress_meter.target = float(uncompressed_size)
                extract_progress_meter.report_eta = False
                extract_progress_meter.report_speed = False

                extract_progress_meter.start()

                zip_file_info = zip_file.infolist()

                for file_path in zip_file_info:
                    zip_file.extract(file_path, path=unpack_dir_path)
                    extract_progress_meter.record_progress(file_path.file_size)

                extract_progress_meter.stop()

            except Exception:
                raise Exception("Failed to extract files from {0}. ".format(files))
            finally:
                zip_file.close()
                multi_file_zip.close()

            num_unpacked_files = 0
            for root, dirs, files in os.walk(unpack_dir_path):
                num_unpacked_files += len(files)

            # move temp to
            print "Moving zip contents to final location."
            copy_directory_contents(args, unpack_dir_path, abs_root_dir, uncompressed_size)

        except (shutil.Error, WindowsError, DistutilsFileError) as removeError:
            print ("{0}: {1}").format(type(removeError).__name__, removeError)
            print ("Failed to remove files that already existed at {} before unpacking. Please ensure the files are"
                   " deletable by closing related applications (such as Asset Processor, and the Lumberyard"
                   " Editor), then try running this program again.").format(abs_root_dir)
            script_succeed = False

        except Exception as e:
            print ("Failed to finish acquiring needed files: {} " + TRY_AGAIN_STRING).format(e)
            script_succeed = False

        else:
            remove_downloaded_files = True
            script_succeed = True

        finally:
            # clean up temp dir
            if not args.keep:

                if remove_downloaded_files and os.path.exists(working_dir_path):
                    # printing a line new to have a separation from the other logs
                    print ("\nCleaning up temp files")
                    shutil.rmtree(working_dir_path)
                elif os.path.exists(unpack_dir_path):
                    # printing a line new to have a separation from the other logs
                    print ("\nCleaning up temp files")
                    shutil.rmtree(unpack_dir_path)

    except KeyboardInterrupt:
        print ("\nOperation aborted. Please perform manual cleanup, or re-run git_bootstrap.exe.\n\n")
        sys.stdout.flush()
        sys.exit(0)

if __name__ == "__main__":
    main()
