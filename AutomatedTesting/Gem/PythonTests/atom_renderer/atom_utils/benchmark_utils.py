"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import azlmbr.atom
import azlmbr.legacy.general as general

FOLDER_PATH = '@user@/Scripts/PerformanceBenchmarks'
METADATA_FILE = 'benchmark_metadata.json'

class BenchmarkHelper(object):
    """
    A helper to capture benchmark data.
    """
    def __init__(self, benchmark_name):
        super().__init__()
        self.benchmark_name = benchmark_name
        self.output_path = f'{FOLDER_PATH}/{benchmark_name}'
        self.done = False
        self.capturedData = False
        self.max_frames_to_wait = 200

    def capture_benchmark_metadata(self):
        """
        Capture benchmark metadata and block further execution until it has been written to the disk.
        """
        self.handler = azlmbr.atom.ProfilingCaptureNotificationBusHandler()
        self.handler.connect()
        self.handler.add_callback('OnCaptureBenchmarkMetadataFinished', self.on_data_captured)

        self.done = False
        self.capturedData = False
        success = azlmbr.atom.ProfilingCaptureRequestBus(
            azlmbr.bus.Broadcast, "CaptureBenchmarkMetadata", self.benchmark_name, f'{self.output_path}/{METADATA_FILE}'
        )
        if success:
            self.wait_until_data()
            general.log('Benchmark metadata captured.')
        else:
            general.log('Failed to capture benchmark metadata.')
        return self.capturedData

    def capture_pass_timestamp(self, frame_number):
        """
        Capture pass timestamps and block further execution until it has been written to the disk.
        """
        self.handler = azlmbr.atom.ProfilingCaptureNotificationBusHandler()
        self.handler.connect()
        self.handler.add_callback('OnCaptureQueryTimestampFinished', self.on_data_captured)

        self.done = False
        self.capturedData = False
        success = azlmbr.atom.ProfilingCaptureRequestBus(
            azlmbr.bus.Broadcast, "CapturePassTimestamp", f'{self.output_path}/frame{frame_number}_timestamps.json')
        if success:
            self.wait_until_data()
            general.log('Pass timestamps captured.')
        else:
            general.log('Failed to capture pass timestamps.')
        return self.capturedData

    def capture_cpu_frame_time(self, frame_number):
        """
        Capture CPU frame times and block further execution until it has been written to the disk.
        """
        self.handler = azlmbr.atom.ProfilingCaptureNotificationBusHandler()
        self.handler.connect()
        self.handler.add_callback('OnCaptureCpuFrameTimeFinished', self.on_data_captured)

        self.done = False
        self.capturedData = False
        success = azlmbr.atom.ProfilingCaptureRequestBus(
            azlmbr.bus.Broadcast, "CaptureCpuFrameTime", f'{self.output_path}/cpu_frame{frame_number}_time.json')
        if success:
            self.wait_until_data()
            general.log('CPU frame time captured.')
        else:
            general.log('Failed to capture CPU frame time.')
        return self.capturedData

    def on_data_captured(self, parameters):
        # the parameters come in as a tuple
        if parameters[0]:
            general.log('Captured data successfully.')
            self.capturedData = True
        else:
            general.log('Failed to capture data.')
        self.done = True
        self.handler.disconnect()

    def wait_until_data(self):
        frames_waited = 0
        while self.done == False:
            general.idle_wait_frames(1)
            if frames_waited > self.max_frames_to_wait:
                general.log('Timed out while waiting for the data to be captured')
                self.handler.disconnect()
                break
            else:
                frames_waited = frames_waited + 1
        general.log(f'(waited {frames_waited} frames)')
