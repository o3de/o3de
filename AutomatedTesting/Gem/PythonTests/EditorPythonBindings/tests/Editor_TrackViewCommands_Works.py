#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
def check_result(result, msg):
    from editor_python_test_tools.utils import Report
    if not result:
        Report.result(msg, False)
        raise Exception(msg + " : FAILED")

def Editor_TrackViewCommands_Works():
    # Description: 
    # Tests the track view

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.track_view as track_view
    import azlmbr.legacy.general as general

    # Required for automated tests
    TestHelper.init_idle()

    # Open the test level
    TestHelper.open_level(directory="", level="Base")
    azlmbr.legacy.general.idle_wait_frames(1)

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

    # all tests worked
    Report.result("Editor_TrackViewCommands_Works ran", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_TrackViewCommands_Works)


