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
