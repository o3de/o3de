/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ::IO::Internal
{
    // Report missing files in the archive when they are loaded
    void ReportFileMissingFromArchive(const char *szPath);
}
