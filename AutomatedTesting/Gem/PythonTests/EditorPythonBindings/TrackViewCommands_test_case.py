#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Tests the track view

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.track_view as track_view
import azlmbr.legacy.general as general


print ('Start of track view tests.')

general.idle_wait(1.0)

editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'OpenLevelNoPrompt', 'ocean_trackview')

general.idle_wait(1.0)

num_sequences = track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'GetNumSequences')

test_sequence_name = 'Test Sequence 01'
track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'NewSequence', test_sequence_name, 1)

new_num_sequences = track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'GetNumSequences')
if new_num_sequences == num_sequences + 1:
    print('[PASS] GetNumSequences returned the expected number of sequences [{}].'.format(new_num_sequences))
else:
    print('[FAIL] GetNumSequences returned an unexpected number of sequences, was [{}] and expected [{}].'.
          format(new_num_sequences, num_sequences + 1))

returned_name = track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'GetSequenceName', 0)
if returned_name == test_sequence_name:
    print('[PASS] GetSequenceName returned the expected name [{}]'.format(test_sequence_name))
else:
    print('[FAIL] GetSequenceName returned an unexpected name, was [{}] and expected [{}].'.
          format(returned_name, test_sequence_name))

# Test modifying the sequence time range
time_range = track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'GetSequenceTimeRange', test_sequence_name)
start = time_range.start + 5.0
end = time_range.end + 10.0
track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'SetSequenceTimeRange', test_sequence_name, start, end)
new_time_range = track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'GetSequenceTimeRange', test_sequence_name)

if (int(new_time_range.start) != int(time_range.start)) and (int(new_time_range.end) != int(time_range.end)):
    print('[PASS] SetSequenceTimeRange modified time range successfully.')
else:
    print('[FAIL] SetSequenceTimeRange did not modify the time range as expected.')

test_sequence_name_2 = 'Test Sequence 02'
track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'NewSequence', test_sequence_name_2, 1)
track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'DeleteSequence', test_sequence_name_2)
print('[PASS] DeleteSequence ran with no error thrown.')

track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'SetCurrentSequence', test_sequence_name)
print('[PASS] SetCurrentSequence ran with no error thrown.')

track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'PlaySequence')
print('[PASS] PlaySequence ran with no error thrown.')

track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'AddNode', 'Director', 'Test Director')
test_node_name = 'Test Node 01'
track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'AddNode', 'Event', test_node_name)
print('[PASS] AddNode ran with no error thrown.')

new_num_nodes = track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'GetNumNodes', '')
num_expected_nodes = 2
if new_num_nodes == num_expected_nodes:
    print('[PASS] Found the expected number of nodes [{}].'.format(num_expected_nodes))
else:
    print('[FAIL] Found [{}] instead of [{}] nodes.'.format(new_num_nodes, num_expected_nodes))

returned_node_name = track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'GetNodeName', 1, '')
if returned_node_name == test_node_name:
    print('[PASS] GetNodeName returned the expected name [{}].'.format(test_node_name))
else:
    print('[FAIL] GetNodeName returned [{}] and expected [{}].'.format(test_node_name, returned_node_name))

track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'DeleteNode', test_node_name, '')
print('[PASS] DeleteNode ran with no error thrown.')

new_num_nodes = track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'GetNumNodes', '')
num_expected_nodes = 1
if new_num_nodes == num_expected_nodes:
    print('[PASS] Found the expected number of nodes [{}].'.format(num_expected_nodes))
else:
    print('[FAIL] Found [{}] instead of [{}] nodes.'.format(new_num_nodes, num_expected_nodes))

track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'DeleteSequence', test_sequence_name)
print('[PASS] DeleteSequence ran with no error thrown.')

print ('End of track view tests.')
