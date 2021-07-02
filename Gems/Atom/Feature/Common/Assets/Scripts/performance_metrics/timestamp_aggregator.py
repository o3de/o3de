#!/usr/bin/env python
"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from argparse import ArgumentParser
import json
from pathlib import Path
import time
import copy
import subprocess

# this allows us to add additional data if necessary, e.g. frame_test_timestamps.json
is_timestamp_file = lambda file: file.name.startswith('frame') and file.name.endswith('_timestamps.json')
ns_to_ms = lambda time: time / 1e6

def main(benchmark_dir, test_suite):
    start_timestamp = time.time()

    # TODO: Change cwd=benchmark_dir to o3de directory based on pytest arguments, since
    # this gives commit hash of ASV not Atom
    git_commit_data = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD'], cwd=benchmark_dir)

    metadata_file = benchmark_dir / 'benchmark_metadata.json'
    if metadata_file.exists():
        data = json.loads(metadata_file.read_text())
        benchmark_metadata = data['ClassData']
    else:
        raise FileNotFoundError(f'Metadata file could not be found at {metadata_file}')
    benchmark_metadata['gitCommit'] = git_commit_data.decode('ascii').strip()

    filebeat_dir = benchmark_dir / 'filebeat'
    filebeat_dir.mkdir(parents=True, exist_ok=True)

    frame_stats = { 'count': 0, 'totalTime': 0, 'maxTime': 0, 'minTime': float('inf') }
    pass_stats = {}

    for file in benchmark_dir.iterdir():
        if file.is_dir() or not is_timestamp_file(file):
            continue

        data = json.loads(file.read_text())
        entries = data['ClassData']['timestampEntries']

        frame_time = 0
        for entry in entries:
            name = entry['passName']
            time_ns = entry['timestampResultInNanoseconds']
            pass_entry = pass_stats.get(name, { 'totalTime': 0, 'maxTime': 0 })

            pass_entry['maxTime'] = max(time_ns, pass_entry['maxTime'])
            pass_entry['totalTime'] += time_ns
            pass_stats[name] = pass_entry

            frame_time += time_ns
        frame_stats['totalTime'] += frame_time

        frame_stats['maxTime'] = max(frame_time, frame_stats['maxTime'])
        frame_stats['minTime'] = min(frame_time, frame_stats['minTime'])
        frame_stats['count'] += 1

    if frame_stats['count'] < 1:
        raise FileNotFoundError(f'No frame timestamp logs were found in {benchmark_dir}')

    frame_time_avg = frame_stats['totalTime'] / frame_stats['count']

    frame_payload = copy.deepcopy(benchmark_metadata)
    frame_payload['frameTime'] = {
        'avg': ns_to_ms(frame_time_avg),
        'max': ns_to_ms(frame_stats['maxTime']),
        'min': ns_to_ms(frame_stats['minTime'])
    }
    data = {
        'index': f'ly_atom.performance_metrics.{test_suite}.frame_data',
        'pipeline': 'filebeat',
        'timestamp': start_timestamp,
        'payload': json.dumps(frame_payload)
    }

    output_loc = filebeat_dir / 'frame_stats.json'
    with open(output_loc, 'w') as output_file:
        json.dump(data, output_file)

    for name, stat in pass_stats.items():
        avg_ms = ns_to_ms(stat['totalTime'] / frame_stats['count'])
        max_ms = ns_to_ms(stat['maxTime'])

        pass_payload = copy.deepcopy(benchmark_metadata)
        pass_payload['name'] = name
        pass_payload['passTime'] = {
            'avg': avg_ms,
            'max': max_ms
        }
        data = {
            'index': f'ly_atom.performance_metrics.{test_suite}.pass_data',
            'pipeline': 'filebeat',
            'timestamp': start_timestamp,
            'payload': json.dumps(pass_payload)
        }

        output_loc = filebeat_dir / f'{name}_stats.json'
        with open(output_loc, 'w') as output_file:
            json.dump(data, output_file)

if __name__ == '__main__':
    parser = ArgumentParser(description='Aggregates benchmark data and serializes them into a MARS-acceptable format.')
    parser.add_argument('path', help='Path to the directory containing the benchmark data')
    parser.add_argument('--suite', help='Name of the test suite the benchmark was run under (default: sandbox)', default='sandbox')
    args = parser.parse_args()

    base_dir = Path(args.path)
    if not base_dir.exists():
        raise FileNotFoundError('Invalid path provided')
    main(base_dir, args.suite)
