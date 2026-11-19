#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

set(FILES
    "${miniaudio_SOURCE_DIR}/extras/decoders/libopus/miniaudio_libopus.c"
    "${miniaudio_SOURCE_DIR}/extras/decoders/libopus/miniaudio_libopus.h"
    "${miniaudio_SOURCE_DIR}/extras/decoders/libvorbis/miniaudio_libvorbis.c"
    "${miniaudio_SOURCE_DIR}/extras/decoders/libvorbis/miniaudio_libvorbis.h"
    "${miniaudio_SOURCE_DIR}/extras/nodes/ma_channel_combiner_node/ma_channel_combiner_node.c"
    "${miniaudio_SOURCE_DIR}/extras/nodes/ma_channel_combiner_node/ma_channel_combiner_node.h"
    "${miniaudio_SOURCE_DIR}/extras/nodes/ma_channel_separator_node/ma_channel_separator_node.c"
    "${miniaudio_SOURCE_DIR}/extras/nodes/ma_channel_separator_node/ma_channel_separator_node.h"
    "${miniaudio_SOURCE_DIR}/extras/nodes/ma_ltrim_node/ma_ltrim_node.c"
    "${miniaudio_SOURCE_DIR}/extras/nodes/ma_ltrim_node/ma_ltrim_node.h"
    "${miniaudio_SOURCE_DIR}/extras/nodes/ma_reverb_node/ma_reverb_node.c"
    "${miniaudio_SOURCE_DIR}/extras/nodes/ma_reverb_node/ma_reverb_node.h"
    "${miniaudio_SOURCE_DIR}/extras/nodes/ma_reverb_node/verblib.h"
    "${miniaudio_SOURCE_DIR}/extras/nodes/ma_vocoder_node/ma_vocoder_node.c"
    "${miniaudio_SOURCE_DIR}/extras/nodes/ma_vocoder_node/ma_vocoder_node.h"
    "${miniaudio_SOURCE_DIR}/extras/nodes/ma_vocoder_node/voclib.h"
    "${miniaudio_SOURCE_DIR}/miniaudio.c"
    "${miniaudio_SOURCE_DIR}/miniaudio.h"
)
