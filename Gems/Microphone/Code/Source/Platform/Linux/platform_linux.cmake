#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Microphone Gem on Linux uses the platform-agnostic "None" stub
# (Source/Platform/None/MicrophoneSystemComponent_None.cpp -- a do-nothing
# implementation). The stub doesn't call any libsamplerate function, so the
# 3rdParty::libsamplerate dependency and USE_LIBSAMPLERATE define are skipped
# on Linux to avoid a dependency that's never exercised at runtime.
set(PAL_TRAIT_MICROPHONE_USES_LIBSAMPLERATE FALSE)
