/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace OpenXRVk::Platform
{
    //! Initializes the XR loader for this platform.
    bool OpenXRInitializeLoader();

    //! Called when the device is beginning a frame for processing.
    //! @note This function is called from the thread related to the presentation queue.
    void OpenXRBeginFrameInternal();

    //! Called when the device is ending a frame for processing.
    //! @note This function is called from the thread related to the presentation queue.
    void OpenXREndFrameInternal();

    //! Called after the EndFrame has been executed.
    //! @note This function is called from the main thread.
    void OpenXRPostFrameInternal();

} // namespace OpenXRVk::Platform
