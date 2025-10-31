/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <QMouseEvent>
#endif
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/utils.h>
#include <Window/ParticleCommonData.h>

namespace OpenParticleSystemEditor
{
    class ParticleLineWidget : public QWidget
    {
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
