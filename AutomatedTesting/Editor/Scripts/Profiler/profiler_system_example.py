#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import azlmbr.debug as debug

import pathlib

def test_profiler_system():
    if not debug.g_ProfilerSystem.IsValid():
        print('g_ProfilerSystem is INVALID')
        return

    state = 'ACTIVE' if debug.g_ProfilerSystem.IsActive() else 'INACTIVE'
    print(f'Profiler system is currently {state}')

    capture_location = pathlib.Path(debug.g_ProfilerSystem.GetCaptureLocation())
    print(f'Capture location set to {capture_location}')

    print('Capturing single frame...' )
    capture_file = str(capture_location / 'script_capture_frame.json')
    debug.g_ProfilerSystem.CaptureFrame(capture_file)

# Invoke main function
if __name__ == '__main__':
   test_profiler_system()
