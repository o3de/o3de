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
            class SharedPreviewRendererContext;

            //! SharedPreviewRendererState decouples SharedPreviewRenderer logic into easy-to-understand and debug pieces
            class SharedPreviewRendererState
            {
            public:
                explicit SharedPreviewRendererState(SharedPreviewRendererContext* context) : m_context(context) {}
                virtual ~SharedPreviewRendererState() = default;

                //! Start is called when step begins execution
                virtual void Start() {}
                //! Stop is called when step ends execution
                virtual void Stop() {}

            protected:
                SharedPreviewRendererContext* m_context;
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

