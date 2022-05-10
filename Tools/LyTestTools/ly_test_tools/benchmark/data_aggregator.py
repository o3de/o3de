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
import os

from ly_test_tools.mars.filebeat_client import FilebeatClient

class BenchmarkPathException(Exception):
    """Custom Exception class for invalid benchmark file paths."""
    pass

class RunningStatistics(object):
    def __init__(self):
        '''
        Initializes a helper class for calculating running statstics.
        '''
        self.count = 0
        self.total = 0
        self.max = 0
        self.min = float('inf')

    def update(self, value):
        '''
        Updates the statistics with a new value.

        :param value: The new value to update the statistics with.
        '''
        self.total += value
        self.count += 1
        self.max = max(value, self.max)
        self.min = min(value, self.min)

    def getAvg(self):
        '''
        Returns the average of the running values.
        '''
        return self.total / self.count

    def getMax(self):
        '''
        Returns the maximum of the running values.
        '''
        return self.max

    def getMin(self):
        '''
        Returns the minimum of the running values.
        '''
        return self.min

    def getCount(self):
        return self.count

class BenchmarkDataAggregator(object):
    def __init__(self, workspace, logger, test_suite):
        '''
        Initializes an aggregator for benchmark data.

        :param workspace: Workspace of the test suite the benchmark was run in
        :param logger: Logger used by the test suite the benchmark was run in
        :param test_suite: Name of the test suite the benchmark was run in
        '''
        self.build_dir = workspace.paths.build_directory()
        self.results_dir = Path(workspace.paths.project(), 'user/Scripts/PerformanceBenchmarks')
        self.test_suite = test_suite if os.environ.get('BUILD_NUMBER') else 'local'
        self.filebeat_client = FilebeatClient(logger)

    def _update_pass(self, gpu_pass_stats, entry):
        '''
        Modifies gpu_pass_stats dict keyed by pass name with the time recorded in a pass timestamp entry.

        :param gpu_pass_stats: dict aggregating statistics from each pass (key: pass name, value: dict with stats)
        :param entry: dict representing the timestamp entry of a pass
        :return: Time (in nanoseconds) recorded by this pass
        '''
        name = entry['passName']
        time_ns = entry['timestampResultInNanoseconds']
        pass_entry = gpu_pass_stats.get(name, RunningStatistics())
        pass_entry.update(time_ns)
        gpu_pass_stats[name] = pass_entry
        return time_ns

    def _process_benchmark(self, benchmark_dir, benchmark_metadata):
        '''
        Aggregates data from results from a single benchmark contained in a subdirectory of self.results_dir.

        :param benchmark_dir: Path of directory containing the benchmark results
        :param benchmark_metadata: Dict with benchmark metadata mutated with additional info from metadata file
        :return: Tuple with two indexes:
            [0]: RunningStatistics for GPU frame times
            [1]: Dict aggregating statistics from GPU pass times (key: pass name, value: RunningStatistics)
        '''
        # Parse benchmark metadata
        metadata_file = benchmark_dir / 'benchmark_metadata.json'
        if metadata_file.exists():
            data = json.loads(metadata_file.read_text())
            benchmark_metadata.update(data['ClassData'])
        else:
            raise BenchmarkPathException(f'Metadata file could not be found at {metadata_file}')

        # data structures aggregating statistics from timestamp logs
        gpu_frame_stats = RunningStatistics()
        cpu_frame_stats = RunningStatistics()
        gpu_pass_stats = {}  # key: pass name, value: RunningStatistics

        # this allows us to add additional data if necessary, e.g. frame_test_timestamps.json
        is_timestamp_file = lambda file: file.name.startswith('frame') and file.name.endswith('_timestamps.json')
        is_frame_time_file = lambda file: file.name.startswith('cpu_frame') and file.name.endswith('_time.json')

        # parse benchmark files
        for file in benchmark_dir.iterdir():
            if file.is_dir():
                continue

            if is_timestamp_file(file):
                data = json.loads(file.read_text())
                entries = data['ClassData']['timestampEntries']

                frame_time = sum(self._update_pass(gpu_pass_stats, entry) for entry in entries)
                gpu_frame_stats.update(frame_time)

            if is_frame_time_file(file):
                data = json.loads(file.read_text())
                frame_time = data['ClassData']['frameTime']
                cpu_frame_stats.update(frame_time)

        if gpu_frame_stats.getCount() < 1 and cpu_frame_stats.getCount() < 1:
            raise BenchmarkPathException(f'No benchmark logs were found in {benchmark_dir}')

        return gpu_frame_stats, gpu_pass_stats, cpu_frame_stats

    def _generate_payloads(self, benchmark_metadata, gpu_frame_stats, gpu_pass_stats, cpu_frame_stats):
        '''
        Generates payloads to send to Filebeat based on aggregated stats and metadata.

        :param benchmark_metadata: Dict of benchmark metadata
        :param gpu_frame_stats: RunningStatistics for GPU frame data
        :param gpu_pass_stats: Dict of aggregated pass RunningStatistics
        :param cpu_frame_stats: RunningStatistics for CPU frame data
        :return payloads: List of tuples, each with two indexes:
            [0]: Elasticsearch index suffix associated with the payload
            [1]: Payload dict to deliver to Filebeat
        '''
        ns_to_ms = lambda ns: ns / 1e6
        payloads = []

        # add benchmark metadata to payload
        if (gpu_frame_stats.getCount() > 0):
            # calculate statistics based on aggregated frame data
            gpu_frame_payload = {
                'frameTime': {
                    'avg': ns_to_ms(gpu_frame_stats.getAvg()),
                    'max': ns_to_ms(gpu_frame_stats.getMax()),
                    'min': ns_to_ms(gpu_frame_stats.getMin())
                }
            }

            gpu_frame_payload.update(benchmark_metadata)
            payloads.append(('gpu.frame_data', gpu_frame_payload))

            # calculate statistics for each pass
            for name, stat in gpu_pass_stats.items():
                gpu_pass_payload = {
                    'passName': name,
                    'passTime': {
                        'avg': ns_to_ms(stat.getAvg()),
                        'max': ns_to_ms(stat.getMax())
                    }
                }
                # add benchmark metadata to payload
                gpu_pass_payload.update(benchmark_metadata)
                payloads.append(('gpu.pass_data', gpu_pass_payload))

        if (cpu_frame_stats.getCount() > 0):
            # calculate statistics based on aggregated frame data
            cpu_frame_payload = {
                'frameTime': {
                    'avg': cpu_frame_stats.getAvg(),
                    'max': cpu_frame_stats.getMax(),
                    'min': cpu_frame_stats.getMin()
                }
            }

            cpu_frame_payload.update(benchmark_metadata)
            payloads.append(('cpu.frame_data', cpu_frame_payload))

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
            gpu_frame_stats, gpu_pass_stats, cpu_frame_stats = self._process_benchmark(benchmark_dir, benchmark_metadata)
            payloads = self._generate_payloads(benchmark_metadata, gpu_frame_stats, gpu_pass_stats, cpu_frame_stats)

            for index_suffix, payload in payloads:
                self.filebeat_client.send_event(
                    payload,
                    f'ly_atom.performance_metrics.{self.test_suite}.{index_suffix}',
                    start_timestamp
                )
