/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Base.h>

namespace AtomToolsFramework
{
    struct PreviewRendererCaptureRequest;

    //! Public interface for PreviewRenderer so that it can be used in other modules
    class PreviewRendererInterface
    {
    public:
        AZ_RTTI(PreviewRendererInterface, "{C5B5E3D0-0055-4C08-9B98-FDBBB5F05BED}");

        virtual ~PreviewRendererInterface() = default;
        virtual void AddCaptureRequest(const PreviewRendererCaptureRequest& captureRequest) = 0;
        virtual AZ::RPI::ScenePtr GetScene() const = 0;
        virtual AZ::RPI::ViewPtr GetView() const = 0;
        virtual AZ::Uuid GetEntityContextId() const = 0;
    };
} // namespace AtomToolsFramework
