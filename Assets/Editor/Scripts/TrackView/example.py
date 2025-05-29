#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
'''
This example script prints info of all TrackView sequences
'''

import azlmbr.bus as bus
import azlmbr.track_view as track_view

test_sequence_name = 'Test Sequence 01'

num_sequences = track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'GetNumSequences')

if num_sequences > 0:
    print(f"Found {num_sequences} sequences")
else:
    track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'NewSequence', test_sequence_name, 1)
    print(f"Created new sequence {test_sequence_name}")

num_sequences = track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'GetNumSequences')
print(f"Number of Sequences: {num_sequences}")

for i in range(0, num_sequences):
    returned_name = track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'GetSequenceName', i)
    time_range = track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'GetSequenceTimeRange', returned_name)
    print(f"Sequence {i}: {returned_name} | Start: {time_range.start} End: {time_range.end}")
