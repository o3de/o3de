/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

