/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/PlatformIncl.h>
#include <QOpenGLFunctions>

QT_FORWARD_DECLARE_CLASS(QOpenGLContext);

namespace RenderGL
{
    using _glMapBuffer = void *(APIENTRY *) (GLenum, GLenum);

    struct GLExtensionFunctions
    {
        bool resolve(const QOpenGLContext* context);

        _glMapBuffer glMapBuffer;
    };

} // namespace RenderGL
