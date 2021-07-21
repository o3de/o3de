"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from argparse import ArgumentParser
import json
from pathlib import Path
import time
import subprocess

from ly_test_tools.mars.filebeat_client import FilebeatClient

class BenchmarkPathException(Exception):
    """Custom Exception class for invalid benchmark file paths."""
    pass

class BenchmarkDataAggregator(object):
    def __init__(self, workspace, logger, test_suite):
        self.build_dir = workspace.paths.build_directory()
        self.results_dir = Path(workspace.paths.project(), 'user/Scripts/PerformanceBenchmarks')
        self.test_suite = test_suite
        self.filebeat_client = FilebeatClient(logger)

    def _update_pass(self, pass_stats, entry):
        '''
        Modifies pass_stats dict keyed by pass name with the time recorded in a pass timestamp entry.

        :param pass_stats: dict aggregating statistics from each pass (key: pass name, value: dict with stats)
        :param entry: dict representing the timestamp entry of a pass
        :return: Time (in nanoseconds) recorded by this pass
        '''
        name = entry['passName']
        time_ns = entry['timestampResultInNanoseconds']
        pass_entry = pass_stats.get(name, { 'totalTime': 0, 'maxTime': 0 })

        pass_entry['maxTime'] = max(time_ns, pass_entry['maxTime'])
        pass_entry['totalTime'] += time_ns
        pass_stats[name] = pass_entry
        return time_ns


    def _process_benchmark(self, benchmark_dir, benchmark_metadata):
        '''
        Aggregates data from results from a single benchmark contained in a subdirectory of self.results_dir.

        :param benchmark_dir: Path of directory containing the benchmark results
        :param benchmark_metadata: Dict with benchmark metadata mutated with additional info from metadata file
        :return: Tuple with two indexes:
            [0]: Dict aggregating statistics from frame times (key: stat name)
            [1]: Dict aggregating statistics from pass times (key: pass name, value: dict with stats)
        '''
        # Parse benchmark metadata
        metadata_file = benchmark_dir / 'benchmark_metadata.json'
        if metadata_file.exists():
            data = json.loads(metadata_file.read_text())
            benchmark_metadata.update(data['ClassData'])
        else:
            raise BenchmarkPathException(f'Metadata file could not be found at {metadata_file}')

        # data structures aggregating statistics from timestamp logs
        frame_stats = { 'count': 0, 'totalTime': 0, 'maxTime': 0, 'minTime': float('inf') }
        pass_stats = {}  # key: pass name, value: dict with totalTime and maxTime keys

        # this allows us to add additional data if necessary, e.g. frame_test_timestamps.json
        is_timestamp_file = lambda file: file.name.startswith('frame') and file.name.endswith('_timestamps.json')

        # parse benchmark files
        for file in benchmark_dir.iterdir():
            if file.is_dir() or not is_timestamp_file(file):
                continue

            data = json.loads(file.read_text())
            entries = data['ClassData']['timestampEntries']

            frame_time = sum(self._update_pass(pass_stats, entry) for entry in entries)

            frame_stats['totalTime'] += frame_time
            frame_stats['maxTime'] = max(frame_time, frame_stats['maxTime'])
            frame_stats['minTime'] = min(frame_time, frame_stats['minTime'])
            frame_stats['count'] += 1

        if frame_stats['count'] < 1:
            raise BenchmarkPathException(f'No frame timestamp logs were found in {benchmark_dir}')

        return frame_stats, pass_stats

    def _generate_payloads(self, benchmark_metadata, frame_stats, pass_stats):
        '''
        Generates payloads to send to Filebeat based on aggregated stats and metadata.

        :param benchmark_metadata: Dict of benchmark metadata
        :param frame_stats: Dict of aggregated frame statistics
        :param pass_stats: Dict of aggregated pass statistics
        :return payloads: List of tuples, each with two indexes:
            [0]: Elasticsearch index suffix associated with the payload
            [1]: Payload dict to deliver to Filebeat
        '''
        ns_to_ms = lambda ns: ns / 1e6
        payloads = []

        # calculate statistics based on aggregated frame data
        frame_time_avg = frame_stats['totalTime'] / frame_stats['count']
        frame_payload = {
            'frameTime': {
                'avg': ns_to_ms(frame_time_avg),
                'max': ns_to_ms(frame_stats['maxTime']),
                'min': ns_to_ms(frame_stats['minTime'])
            }
        }
        # add benchmark metadata to payload
        frame_payload.update(benchmark_metadata)
        payloads.append(('frame_data', frame_payload))

        # calculate statistics for each pass
        for name, stat in pass_stats.items():
            avg_ms = ns_to_ms(stat['totalTime'] / frame_stats['count'])
            max_ms = ns_to_ms(stat['maxTime'])

            pass_payload = {
                'passName': name,
                'passTime': {
                    'avg': avg_ms,
                    'max': max_ms
                }
            }
            # add benchmark metadata to payload
            pass_payload.update(benchmark_metadata)
            payloads.append(('pass_data', pass_payload))

        return payloads

    def upload_metrics(self, rhi):
        '''
        Uploads metrics aggregated from all the benchmarks run in a test suite to filebeat.

        :param rhi: The RHI the benchmarks were run on
        '''
        start_timestamp = time.time()

        git_commit_data = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD'], cwd=self.build_dir)
        git_commit_hash = git_commit_data.decode('ascii').strip()
        build_date = time.strftime('%m/%d/%y', time.localtime(start_timestamp))  # use gmtime if GMT is preferred

        for benchmark_dir in self.results_dir.iterdir():
            if not benchmark_dir.is_dir():
                continue

            benchmark_metadata = {
                'gitCommitAndBuildDate': f'{git_commit_hash} {build_date}',
                'RHI': rhi
            }
            frame_stats, pass_stats = self._process_benchmark(benchmark_dir, benchmark_metadata)
            payloads = self._generate_payloads(benchmark_metadata, frame_stats, pass_stats)

            for index_suffix, payload in payloads:
                self.filebeat_client.send_event(
                    payload,
                    f'ly_atom.performance_metrics.{self.test_suite}.{index_suffix}',
                    start_timestamp
                )
