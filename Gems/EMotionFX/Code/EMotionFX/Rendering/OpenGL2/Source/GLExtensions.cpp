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

#include <EMotionFX/Rendering/OpenGL2/Source/GLExtensions.h>
#include <QOpenGLContext>

QT_FORWARD_DECLARE_CLASS(QOpenGLContext);

namespace RenderGL
{
    bool GLExtensionFunctions::resolve(const QOpenGLContext* context)
    {
        bool ok = true;

        ok &= bool((glMapBuffer = (_glMapBuffer)context->getProcAddress(QByteArray("glMapBuffer"))));

        return ok;
    };
} // namespace RenderGL

