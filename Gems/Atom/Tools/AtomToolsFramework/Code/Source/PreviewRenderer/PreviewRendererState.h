/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            class CommonPreviewRenderer;

            //! CommonPreviewRendererState decouples CommonPreviewRenderer logic into easy-to-understand and debug pieces
            class CommonPreviewRendererState
            {
            public:
                explicit CommonPreviewRendererState(CommonPreviewRenderer* renderer) : m_renderer(renderer) {}
                virtual ~CommonPreviewRendererState() = default;

                //! Start is called when state begins execution
                virtual void Start() {}
                //! Stop is called when state ends execution
                virtual void Stop() {}

            protected:
                CommonPreviewRenderer* m_renderer;
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

