/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <PreviewRenderer/PreviewRendererState.h>

namespace AtomToolsFramework
{
    //! PreviewRendererLoadState pauses further rendering until all assets used for rendering a thumbnail have been loaded
    class PreviewRendererLoadState final : public PreviewRendererState
    {
    public:
        PreviewRendererLoadState(PreviewRenderer* renderer);
        ~PreviewRendererLoadState();
        void Update() override;

    private:
        AZStd::chrono::steady_clock::time_point m_startTime = AZStd::chrono::steady_clock::now();
        AZStd::chrono::steady_clock::time_point m_abortTime = AZStd::chrono::steady_clock::now() + AZStd::chrono::milliseconds(5000);
    };
} // namespace AtomToolsFramework
