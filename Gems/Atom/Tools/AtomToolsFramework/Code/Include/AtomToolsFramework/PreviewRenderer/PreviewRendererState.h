/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AtomToolsFramework
{
    class PreviewRenderer;

    //! PreviewRendererState decouples PreviewRenderer logic into easy-to-understand and debug pieces
    class PreviewRendererState
    {
    public:
        explicit PreviewRendererState(PreviewRenderer* renderer)
            : m_renderer(renderer)
        {
        }

        virtual ~PreviewRendererState() = default;

        //! Start is called when state begins execution
        virtual void Start() = 0;

        //! Stop is called when state ends execution
        virtual void Stop() = 0;

    protected:
        PreviewRenderer* m_renderer = {};
    };
} // namespace AtomToolsFramework
