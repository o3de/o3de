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

numTracks = trackview.get_num_sequences()

for i in range(numTracks):
    print "Sequence '", trackview.get_sequence_name(i), "':"