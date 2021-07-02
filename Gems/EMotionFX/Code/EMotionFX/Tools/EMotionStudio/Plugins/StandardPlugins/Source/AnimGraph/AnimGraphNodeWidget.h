/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QPersistentModelIndex>
#include <QWidget>
#endif

namespace EMotionFX
{
    class AnimGraphNode;
}

namespace EMStudio
{
    class AnimGraphNodeWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        AnimGraphNodeWidget(QWidget* parent = nullptr)
            : QWidget(parent)
        {
        }

        virtual ~AnimGraphNodeWidget() = default;

        virtual void SetCurrentNode(EMotionFX::AnimGraphNode* node) = 0;

        void SetCurrentModelIndex(const QPersistentModelIndex& nodeModelIndex) { m_modelIndex = nodeModelIndex; }

    protected:
        QPersistentModelIndex m_modelIndex;
    };
} //namespace EMStudio
