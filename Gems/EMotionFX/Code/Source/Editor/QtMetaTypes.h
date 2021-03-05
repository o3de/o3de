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
