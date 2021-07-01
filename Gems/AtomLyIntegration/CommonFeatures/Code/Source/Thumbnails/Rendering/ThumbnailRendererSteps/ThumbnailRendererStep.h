/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            class ThumbnailRendererContext;

            //! ThumbnailRendererStep decouples CommonThumbnailRenderer logic into easy-to-understand and debug pieces
            class ThumbnailRendererStep
            {
            public:
                explicit ThumbnailRendererStep(ThumbnailRendererContext* context) : m_context(context) {}
                virtual ~ThumbnailRendererStep() = default;

                //! Start is called when step begins execution
                virtual void Start() {}
                //! Stop is called when step ends execution
                virtual void Stop() {}

            protected:
                ThumbnailRendererContext* m_context;
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

