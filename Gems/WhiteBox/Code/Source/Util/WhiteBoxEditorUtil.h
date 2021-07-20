/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

struct IEditor;

namespace WhiteBox
{
    //! Small wrapper around an EBus call to request the file at the given path be added to source control.
    void RequestEditSourceControl(const char* absoluteFilePath);

    //! Small wrapper around an EBus call to retrieve the IEditor interface.
    IEditor* GetIEditor();
} // namespace WhiteBox
