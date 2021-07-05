/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Uuid.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphObjectIds.h>
#include <EMotionFX/Source/AnimGraphObject.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/BlendTreeConnection.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>
#include <QMetaType>
#include <QVector>


// Required to return different types through a QVariant and to make signal/slot connections
Q_DECLARE_METATYPE(AZ::Uuid);
Q_DECLARE_METATYPE(EMotionFX::Actor*);
Q_DECLARE_METATYPE(EMotionFX::ActorInstance*);
Q_DECLARE_METATYPE(EMotionFX::AnimGraphInstance*);
Q_DECLARE_METATYPE(EMotionFX::AnimGraphNode*);
Q_DECLARE_METATYPE(EMotionFX::AnimGraphNodeId);
Q_DECLARE_METATYPE(EMotionFX::AnimGraphObject*);
Q_DECLARE_METATYPE(EMotionFX::AnimGraphStateTransition*);
Q_DECLARE_METATYPE(EMotionFX::BlendTreeConnection*);
Q_DECLARE_METATYPE(EMotionFX::Node*);
Q_DECLARE_METATYPE(EMotionFX::SimulatedObject*);
Q_DECLARE_METATYPE(EMotionFX::SimulatedJoint*);
Q_DECLARE_METATYPE(QVector<int>);
