
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ZoomableLineEdit.h>

namespace EMStudio
{
    ZoomableLineEdit::ZoomableLineEdit(QWidget* parent)
        : QLineEdit(parent)
    {
    }

    void ZoomableLineEdit::setScale(float scale)
    {
        if (!qFuzzyCompare(m_scale, scale))
        {
            m_scale = scale;
            updateScaledSize();
        }
    }

    void ZoomableLineEdit::setBaseSize(QSize sz)
    {
        m_baseSize = sz;
    }

    void ZoomableLineEdit::setBaseFontPointSizeF(qreal pointSize)
    {
        m_fontPointSize = pointSize;
    }

    void ZoomableLineEdit::updateScaledSize()
    {
        QFont f = font();
        f.setPointSizeF(m_fontPointSize * m_scale);
        setFont(f);
        resize(m_baseSize.width() * m_scale, m_baseSize.height() * m_scale);
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_ZoomableLineEdit.cpp>
