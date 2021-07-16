/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QModelIndexList>
#include <QObject>
#endif

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace EMotionFX
{
    class Actor;
}

namespace EMStudio
{
    class SimulatedObjectActionManager
        : public QObject
    {
        Q_OBJECT // AUTOMOC

    public slots:
        /**
         * Creates a new simulated object and adds the given joints to it.
         * @param actor The actor to create the simulated object for.
         * @param selectedJoints Model index list from the skeletal model.
         * @param addChildJoints Automatically add all children for all given joints recursively.
         * @param parent The parent widget.
         */
        void OnAddNewObjectAndAddJoints(EMotionFX::Actor* actor, const QModelIndexList& selectedJoints, bool addChildJoints, QWidget* parent);
    };
} // namespace EMStudio
