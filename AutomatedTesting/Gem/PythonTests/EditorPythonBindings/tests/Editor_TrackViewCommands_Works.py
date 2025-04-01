"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

class Editor_TrackViewCommands_Works(BaseClass):
    # Description: 
    # Tests the track view
    
    @staticmethod
    def test():
        import azlmbr.bus as bus
        import azlmbr.track_view as track_view
        check_result = BaseClass.check_result

        num_sequences = track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'GetNumSequences')
        test_sequence_name = 'Test Sequence 01'
        track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'NewSequence', test_sequence_name, 1)

        new_num_sequences = track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'GetNumSequences')
        check_result(new_num_sequences == num_sequences + 1, f'GetNumSequences returned the sequences [{new_num_sequences}]')

        returned_name = track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'GetSequenceName', 0)
        check_result(returned_name == test_sequence_name, f'GetSequenceName returned the name [{test_sequence_name}]')

        # Test modifying the sequence time range
        time_range = track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'GetSequenceTimeRange', test_sequence_name)
        start = time_range.start + 5.0
        end = time_range.end + 10.0
        track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'SetSequenceTimeRange', test_sequence_name, start, end)
        new_time_range = track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'GetSequenceTimeRange', test_sequence_name)

        result = (int(new_time_range.start) != int(time_range.start)) and (int(new_time_range.end) != int(time_range.end))
        check_result(result, 'SetSequenceTimeRange modified time range')

        test_sequence_name_2 = 'Test Sequence 02'
        track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'NewSequence', test_sequence_name_2, 1)
        track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'DeleteSequence', test_sequence_name_2)
        track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'SetCurrentSequence', test_sequence_name)
        track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'PlaySequence')

        track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'AddNode', 'Director', 'Test Director')
        test_node_name = 'Test Node 01'
        track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'AddNode', 'Event', test_node_name)

        new_num_nodes = track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'GetNumNodes', '')
        num_expected_nodes = 2
        check_result(new_num_nodes == num_expected_nodes, f'Found the number of nodes [{num_expected_nodes}]')

        returned_node_name = track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'GetNodeName', 1, '')
        check_result(returned_node_name == test_node_name, f'GetNodeName returned the name [{test_node_name}]')

        track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'DeleteNode', test_node_name, '')

        new_num_nodes = track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'GetNumNodes', '')
        num_expected_nodes = 1
        check_result(new_num_nodes == num_expected_nodes, f'Found the expected number of nodes [{num_expected_nodes}]')

        track_view.EditorLayerTrackViewRequestBus(bus.Broadcast, 'DeleteSequence', test_sequence_name)

if __name__ == "__main__":
    tester = Editor_TrackViewCommands_Works()
    tester.test_case(tester.test)
