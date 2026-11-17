#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

set(FILES
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_averages_and_directions.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_block_sizes.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_color_quantize.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_color_unquantize.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_compress_symbolic.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_compute_variance.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_decompress_symbolic.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_diagnostic_trace.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_entry.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_find_best_partitioning.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_ideal_endpoints_and_weights.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_image.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_integer_sequence.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_mathlib_softfloat.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_mathlib.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_partition_tables.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_percentile_tables.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_pick_best_endpoint_format.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_platform_isa_detection.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_quantization.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_symbolic_physical.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_weight_align.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc_weight_quant_xfer_tables.cpp"
    "${astc-encoder_SOURCE_DIR}/Source/astcenc.h"
)
