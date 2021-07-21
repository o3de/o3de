#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import datetime
import json
import math
import os
import platform
import psutil
import shutil
import stat
import subprocess
import sys
import time
import ci_build
from pathlib import Path
import submit_metrics

if platform.system() == 'Windows':
    EXE_EXTENSION = '.exe'
else:
    EXE_EXTENSION = ''

DATE_FORMAT = "%Y-%m-%dT%H:%M:%S"

METRICS_TEST_MODE = os.environ.get('METRICS_TEST_MODE')

def parse_args():
    cur_dir = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--platform', dest="platform", help="Platform to gather metrics for")
    parser.add_argument('-r', '--repository', dest="repository", help="Repository to gather metrics for")
    parser.add_argument('-a', '--jobname', dest="jobname", default="unknown", help="Name/tag of the job in the CI system (used to track where the report comes from, constant through multiple runs)")
    parser.add_argument('-u', '--jobnumber', dest="jobnumber", default=-1, help="Number of run in the CI system (used to track where the report comes from, variable through runs)")
    parser.add_argument('-o', '--jobnode', dest="jobnode", default="unknown", help="Build node name (used to track where the build happened in CI systems where the same jobs run in different hosts)")
    parser.add_argument('-l', '--changelist', dest="changelist", default=-1, help="Last changelist in this workspace")
    parser.add_argument('-c', '--config', dest="build_config_filename", default="build_config.json",
                        help="JSON filename in Platform/<platform> that defines build configurations for the platform")
    args = parser.parse_args()

    # Input validation
    if args.platform is None:
        print('[ci_build_metrics] No platform specified')
        sys.exit(-1)

    return args

def shutdown_processes():
    process_list = {f'AssetBuilder{EXE_EXTENSION}', f'AssetProcessor{EXE_EXTENSION}', f'RC{EXE_EXTENSION}', f'Editor{EXE_EXTENSION}'}
    
    for process in psutil.process_iter():
        try:
            if process.name() in process_list:
                process.kill()
        except Exception as e:
            print(f'Exception while trying to kill process: {e}')

def on_rmtree_error(func, path, exc_info):
    if os.path.exists(path):
        try:
            os.chmod(path, stat.S_IWRITE)
            os.unlink(path)
        except Exception as e:
            print(f'Exception while trying to remove {path}: {e}')

def clean_folder(folder):
    if os.path.exists(folder):
        print(f'[ci_build_metrics] Cleaning {folder}...', flush=True)
        if not METRICS_TEST_MODE:
            shutil.rmtree(folder, onerror=on_rmtree_error)
        print(f'[ci_build_metrics] Cleaned {folder}', flush=True)

def compute_folder_size(folder):
    total = 0
    if os.path.exists(folder):
        folder_path = Path(folder)
        print(f'[ci_build_metrics] Computing size of {folder_path}...', flush=True)
        if not METRICS_TEST_MODE:
            total += sum(f.stat().st_size for f in folder_path.glob('**/*') if f.is_file())
        print(f'[ci_build_metrics] Computed size of {folder_path}', flush=True)
    return total

def build(metrics, folders_of_interest, build_config_filename, platform, build_type, output_directory, time_delta = 0):
    build_start = time.time()
    if not METRICS_TEST_MODE:
        metrics['result'] = ci_build.build(build_config_filename, platform, build_type)
    else:
        metrics['result'] = -1 # mark as failure so the data is not used in elastic
    build_end = time.time()
    
    # truncate the duration (expressed in seconds), we dont need more precision
    metrics['duration'] = math.trunc(build_end - build_start - time_delta)

    metrics['output_sizes'] = []
    output_sizes = metrics['output_sizes']
    for folder in folders_of_interest:
        output_size = dict()
        if folder != output_directory:
            output_size['folder'] = folder
        else:
            output_size['folder'] = 'OUTPUT_DIRECTORY' # make it homogenous to facilitate search
        output_size['output_size'] = compute_folder_size(os.path.join(engine_dir, folder))
        output_sizes.append(output_size)

def gather_build_metrics(current_dir, build_config_filename, platform):
    config_dir = os.path.abspath(os.path.join(current_dir, 'Platform', platform))
    build_config_abspath = os.path.join(config_dir, build_config_filename)
    if not os.path.exists(build_config_abspath):
        cwd_dir = os.path.abspath(os.path.join(current_dir, '../..')) # engine's root
        config_dir = os.path.abspath(os.path.join(cwd_dir, 'restricted', platform, os.path.relpath(current_dir, cwd_dir)))
        build_config_abspath = os.path.join(config_dir, build_config_filename)

    if not os.path.exists(build_config_abspath):
        print(f'[ci_build_metrics] File: {build_config_abspath} not found', flush=True)
        sys.exit(-1)

    with open(build_config_abspath) as f:
        build_config_json = json.load(f)

    all_builds_metrics = []

    for build_type in build_config_json:
        
        build_config = build_config_json[build_type]
        if not 'weekly-build-metrics' in build_config['TAGS']:
            # skip build configs that are not tagged with 'weekly-build-metrics'
            continue

        print(f'[ci_build_metrics] Starting {build_type}', flush=True)
        metrics = dict()
        metrics['build_type'] = build_type

        build_parameters = build_config['PARAMETERS']
        if not build_parameters:
            metrics['result'] = -1
            reason = f'PARAMETERS entry {build_type} in {build_config_abspath} is missing.'
            metrics['reason'] = reason
            print(f'[ci_build_metrics] {reason}', flush=True)
            continue

        # Clean the build output
        output_directory = build_parameters['OUTPUT_DIRECTORY'] if 'OUTPUT_DIRECTORY' in build_parameters else None
        if not output_directory:
            metrics['result'] = -1 
            reason = f'OUTPUT_DIRECTORY entry in {build_config_abspath} is missing.'
            metrics['reason'] = reason
            print(f'[ci_build_metrics] {reason}', flush=True)
            continue
        folders_of_interest = [output_directory]

        # Clean the AP output
        cmake_ly_projects = build_parameters['CMAKE_LY_PROJECTS'] if 'CMAKE_LY_PROJECTS' in build_parameters else None
        if cmake_ly_projects:
            projects = cmake_ly_projects.split(';')
            for project in projects:
                folders_of_interest.append(os.path.join(project, 'user', 'AssetProcessorTemp'))
                folders_of_interest.append(os.path.join(project, 'Cache'))
        
        metrics['build_metrics'] = []
        build_metrics = metrics['build_metrics']

        # Do the clean build
        shutdown_processes()
        for folder in folders_of_interest:
            clean_folder(os.path.join(engine_dir, folder))

        build_metric_clean = dict()
        build_metric_clean['build_metric'] = 'clean'
        build(build_metric_clean, folders_of_interest, build_config_filename, platform, build_type, output_directory)
        build_metrics.append(build_metric_clean)

        # Do the incremental "zero" build
        build_metric_zero = dict()
        build_metric_zero['build_metric'] = 'zero'
        build(build_metric_zero, folders_of_interest, build_config_filename, platform, build_type, output_directory)
        build_metrics.append(build_metric_zero)

        # Do a reconfigure
        # To measure a reconfigure, we will delete the "ci_last_configure_cmd.txt" file from the output and trigger a
        # zero build, then we will substract the time from the zero_build above
        last_configure_file = os.path.join(output_directory, 'ci_last_configure_cmd.txt')
        if os.path.exists(last_configure_file):
            os.remove(last_configure_file)
        build_metric_generation = dict()
        build_metric_generation['build_metric'] = 'generation'
        build(build_metric_generation, folders_of_interest, build_config_filename, platform, build_type, output_directory, build_metric_zero['duration'])        
        build_metrics.append(build_metric_generation)

        # Clean the otuput before ending to reduce the size of these workspaces
        shutdown_processes()
        for folder in folders_of_interest:
            clean_folder(os.path.join(engine_dir, folder))

        metrics['result'] = 0
        metrics['reason'] = 'OK'

        all_builds_metrics.append(metrics)

    return all_builds_metrics

def prepare_metrics(args, build_metrics):
    return {
        'changelist': args.changelist,
        'job': {'name': args.jobname, 'number': args.jobnumber, 'node': args.jobnode},
        'platform': args.platform,
        'repository': args.repository,
        'build_types': build_metrics,
        'timestamp': timestamp.strftime("%Y-%m-%dT%H:%M:%S")
    }

def upload_to_s3(upload_script_path, base_dir, bucket, key_prefix):
    try:
        subprocess.run([sys.executable, upload_script_path, 
            '--base_dir', base_dir, 
            '--file_regex', '.*',
            '--bucket', bucket, 
            '--key_prefix', key_prefix],
            check=True)
    except subprocess.CalledProcessError as err:
        print(f'[ci_build_metrics] {upload_script_path} failed with error {err}')
        sys.exit(1)

def submit_report_document(report_file):
    print(f'[ci_build_metrics] Submitting {report_file}')
    with open(report_file) as json_file:
        report_json = json.load(json_file)

    ret = True
    for build_type in report_json['build_types']:
        for build_metric in build_type['build_metrics']:
            newjson = {
                'timestamp': report_json['timestamp'],
                'changelist': report_json['changelist'],
                'job': report_json['job'],
                'platform': report_json['platform'],
                'repository': report_json['repository'],
                'type': build_type['build_type'],
                'result': int(build_type['result']) or int(build_metric['result']),
                'reason': build_type['reason'],
                'metric': build_metric['build_metric'],
                'duration': build_metric['duration'],
                'output_sizes': build_metric['output_sizes']
            }

            index = "pappeste.build_metrics." + datetime.datetime.strptime(report_json['timestamp'], DATE_FORMAT).strftime("%Y.%m")
            ret &= submit_metrics.submit(index, newjson)

    if ret:
        print(f'[ci_build_metrics] {report_file} submitted')
    else:
        print(f'[ci_build_metrics] {report_file} failed to submit')
    return ret

if __name__ == "__main__":
    args = parse_args()
    print(f"[ci_build_metrics] Generating build metrics for:"
        f"\n\tPlatform: {args.platform}"
        f"\n\tRepository: {args.repository}"
        f"\n\tJob Name: {args.jobname}"
        f"\n\tJob Number: {args.jobnumber}"
        f"\n\tJob Node: {args.jobnode}"
        f"\n\tChangelist: {args.changelist}")

    # Read build_config
    current_dir = os.path.dirname(os.path.abspath(__file__))
    engine_dir = os.path.abspath(os.path.join(current_dir, '../..')) # engine's root
    timestamp =  datetime.datetime.now()

    build_metrics = gather_build_metrics(current_dir, args.build_config_filename, args.platform)
    metrics = prepare_metrics(args, build_metrics)

    # Temporarly just printing the metrics until we get an API to uplaod it to CloudWatch
    # SPEC-1810 will then upload these metrics
    print("[ci_build_metrics] metrics:")
    print(json.dumps(metrics, sort_keys=True, indent=4))

    metric_file_path = os.path.join(engine_dir, 'build_metrics')
    if os.path.exists(metric_file_path):
        shutil.rmtree(metric_file_path)
    os.makedirs(metric_file_path)

    metric_file_path = os.path.join(metric_file_path, timestamp.strftime("%Y%m%d_%H%M%S.json"))

    with open(metric_file_path, 'w') as metric_file:
        json.dump(metrics, metric_file, sort_keys=True, indent=4)

    # transfer
    upload_script = os.path.join(current_dir, 'tools', 'upload_to_s3.py')
    upload_to_s3(upload_script, os.path.join(engine_dir, 'build_metrics'), 'ly-jenkins-cmake-metrics', args.jobname)

    # submit
    submit_report_document(metric_file_path)

    # Dont cleanup, next build will remove the file, but leaving it helps to do some post-build forensics
