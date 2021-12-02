/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Rendering/OpenGL2/Source/GLExtensions.h>
#include <QOpenGLContext>

QT_FORWARD_DECLARE_CLASS(QOpenGLContext);

namespace RenderGL
{
    bool GLExtensionFunctions::resolve(const QOpenGLContext* context)
    {
        bool ok = true;

        ok &= bool((m_glMapBuffer = (_glMapBuffer)context->getProcAddress(QByteArray("glMapBuffer"))));

        return ok;
    };
} // namespace RenderGL

