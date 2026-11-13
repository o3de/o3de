/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QWidget>
#include <QMouseEvent>

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/utils.h>
#include <Window/ParticleCommonData.h>

namespace OpenParticleSystemEditor
{
    class ParticleLineWidget : public QWidget
    {
        // Q_OBJECT gives the class a meta object of its own, so that the flattened GUI's rows sit on this class and not on QWidget
        // (every plain widget would then run their matchers)
        Q_OBJECT
    public:
        explicit ParticleLineWidget(QWidget* parent = nullptr);
        virtual ~ParticleLineWidget();
        void SetLineWidgetParent(QWidget* widget);
        void SetLineIndex(LineWidgetIndex Index);
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

        LineWidgetIndex m_index;
        QWidget* m_parent;
    };
} // namespace OpenParticleSystemEditor
