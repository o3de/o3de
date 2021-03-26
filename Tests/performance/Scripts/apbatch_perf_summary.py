"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

"""
Script is designed to help with asset processor performance testing.

This script is capable of running AssetProcessorBatch.exe and 
calculating how much time did it take to execute it. Also it is
processing ap batch output and grabbing actual asset processing time.

Apart from that script is capable of logging folder size
(files, folders, actual size in bytes).

Usage:
python apbatch_perf_summary.py [-h] {folder_size,run_apbatch}

python apbatch_perf_summary.py folder_size path project -cache

Will show folder size: files, folders and actual size in bytes.
 
build_path: Full path to the build dev directory, e.g. F:\builds\lumberyard-0.0-639162-pc-1985\dev
project: Full project name (e.g. StarterGame or SamplesProject).
-cache: specify if you need to check cache folder instead of source assets.

python apbatch_perf_summary.py run_apbatch build_path platform num_launches -delete_cache

Will launch AssetProcessorBatch.exe num_launches times. Will return average 
running time and average asset processing time.

build_path: Full path to the build dev directory, e.g. F:\builds\lumberyard-0.0-639162-pc-1985\dev
platform: Which platform to launch - one of following: vc141, vc142, mac
num_launches: How many times do you want to launch ap batch.
-delete_cache: specify if you want to delete Cache before each run
"""


import subprocess
import time
import os
import argparse
import test_tools.shared.file_system as fs
import errno


def run_ap_batch(build_path, platform):
    """
    Given a path to build will run ap batch and return total running and processing times.
    :param build_path: Full path to build dev, e.g. F:\builds\lumberyard-0.0-639162-pc-1985\dev
    :param platform: Specify platform where to run apbatch: vc141, vc142 or mac.
    :return: (processing_time, total_running_time) tuple.
    """
    assert os.path.exists(build_path)

    now = time.time()
    process = subprocess.Popen(['AssetProcessorBatch'], cwd=os.path.join(build_path, platform),
                                      shell=True, stdout=subprocess.PIPE)
    for line in iter(process.stdout.readline, ''):
        if 'Total Assets Processing Time' in line:
            processing_time = line.split(':')[2]
    process.wait()
    end = time.time()

    print 'Processing time: {}'.format(processing_time.strip())
    print 'Total time: {}s'.format(end - now)

    return float(processing_time.split('s')[0]), float(end - now)


def run_several_times(build_path, platform, num_launches, erase_cache):
    """
    Given path to build, project name and boolean parameter (whether there is need to delete a cache)
    will run AssetProcessorBatch.exe num_launches and will return average running and processing times.
    :param build_path: Full path to build dev, e.g. F:\builds\lumberyard-0.0-639162-pc-1985\dev
    :param platform: Specify platform where to run apbatch: vc141, vc142 or mac.
    :param num_launches: How many times user needs to launch ap batch.
    :param erase_cache: yes/no or True/False in case user needs Cache folder to be deleted prior to apbatch launch.
    :return: (avg_processing_time, avg_running_time) tuple.
    """
    if not os.path.exists(build_path):
        raise IOError(errno.ENOENT, os.strerror(errno.ENOENT), build_path)

    average_processing_time = 0
    average_total_time = 0

    # running apbatch num_launches times and getting times
    for i in range(num_launches):
        if erase_cache and os.path.exists(os.path.join(build_path, 'Cache')):
            fs.delete([os.path.join(build_path, 'Cache')], False, True)            
        print 'Iteration # {}'.format(i)
        processing_time, total_time = run_ap_batch(build_path, platform)
        average_processing_time += processing_time
        average_total_time += total_time

    # calculating average times
    average_processing_time /= num_launches
    average_total_time /= num_launches

    return average_processing_time, average_total_time


def folder_size(path):
    """
    Given path to a build will calculate folder size.
    :param path: Full path to the folder for which you need folder size info.
    :return: (total_files_count, total_folders_count, total_size_in_bytes).
    """
    total_size = 0
    total_files_count = 0
    total_folder_count = 0

    if not os.path.exists(path):
        raise IOError(errno.ENOENT, os.strerror(errno.ENOENT), path)

    # walking over the folder and calculating files, folder; total files size
    for dirpath, dirnames, filenames in os.walk(path):
        total_folder_count += len(dirnames)
        total_files_count += len(filenames)
        for f in filenames:
            fp = os.path.join(dirpath, f)
            total_size += os.path.getsize(fp)

    return total_files_count, total_folder_count, total_size


def run_apbatch(args):
    """
    Function for argparse command run_apbatch:
    running run_several_times function and printing its results.
    :param args: args.build_path: see run_several_times build_path.
                 args.num_launches: see run_several_times num_launches.
                 args.platform: see run_several_times platform.
                 args.delete: see run_several_times erase_cache.
    :return: None
    """
    platform_bin = {
        'vc141': 'Bin64vc141',
        'vc142': 'Bin64vc142',
        'mac': 'BinMac64'
    }
    running_times = run_several_times(args.build_path, platform_bin[args.platform], args.num_launches, args.delete_cache)
    print '\nAssets processing time: {}s'.format(running_times[0])
    print 'Total running time: {}s'.format(running_times[1])


def print_folder_size(args):
    """
    Function for argparse command folder_size:
    running folder_size function and printing its results.
    :param args: args.build_path: see folder_size path.
                 args.project: specified project which folder will be analyzed.
                 args.cache: yes/no or True/False - whether user need to check Cache folder or not.
    :return: None
    """
    print '{} (cache: {}) folder size:'.format(args.project, args.cache)
    if args.cache:
        path = os.path.join(args.build_path, 'Cache', args.project)
    else:
        path = os.path.join(args.build_path, args.project)
    folder_size_data = folder_size(path)
    print 'Files: {}'.format(folder_size_data[0])
    print 'Folders: {}'.format(folder_size_data[1])
    print 'Size: {}'.format(folder_size_data[2])


def main():
    """Main function with set-up and commands execution"""
    # creating command line arguments parser
    parser = argparse.ArgumentParser(prog = 'apbatch_perf_summary')
    subparsers = parser.add_subparsers(help = 'sub-command help', dest='command')

    parser_folder_size = subparsers.add_parser('folder_size',
                                               help='Will show folder size: files, folders and actual size in bytes. ')
    parser_run_apbatch = subparsers.add_parser('run_apbatch', help='run_apbatch help')

    parser_run_apbatch.add_argument('build_path',
                                    help='Full path to the build dev directory, e.g. '
                                         'F:\\builds\\lumberyard-0.0-639162-pc-1985\\dev')
    parser_run_apbatch.add_argument('platform', choices=['vc141', 'vc142', 'mac'], help='vc141, vc142 or mac')
    parser_run_apbatch.add_argument('num_launches', type=int, help='How many times do you want to launch ap batch.')
    parser_run_apbatch.add_argument('-delete_cache', default=False, action='store_true',
                                    help='Specify if you want to delete Cache before and between runs')

    parser_run_apbatch.set_defaults(func=run_apbatch)

    parser_folder_size.add_argument('build_path',
                                    help='Full path to the build dev directory, e.g. '
                                         'F:\\builds\\lumberyard-0.0-639162-pc-1985\dev')
    parser_folder_size.add_argument('project', help='Full project name (e.g. StarterGame or SamplesProject).')
    parser_folder_size.add_argument('-cache', default=False, action='store_true',
                                    help='Specify if you want to check cache folder', required=False)
    parser_folder_size.set_defaults(func=print_folder_size)

    args = parser.parse_args()

    # executing passed commands
    args.func(args)


# calling main function if script is launched as standalone module
if __name__ == '__main__':
    main()

