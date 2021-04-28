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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_RESOURCES_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_RESOURCES_H
#pragma once
#include "ResourceSelector.h"

namespace Serialization
{
    // animation resources
    template<class T>
    ResourceSelector<T> AnimationAlias(T& s) { return ResourceSelector<T>(s, "AnimationAlias"); }                   // "name" from animation set
    template<class T>
    ResourceSelector<T> AnimationPath(T& s) { return ResourceSelector<T>(s, "Animation"); }
    inline ResourceSelectorWithId AnimationPathWithId(string& s, int id) { return ResourceSelectorWithId(s, "Animation", id); }
    template<class T>
    ResourceSelector<T> CharacterPath(T& s) { return ResourceSelector<T>(s, "Character"); }
    template<class T>
    ResourceSelector<T> CharacterPhysicsPath(T& s) { return ResourceSelector<T>(s, "CharacterPhysics"); }
    template<class T>
    ResourceSelector<T> CharacterRigPath(T& s) { return ResourceSelector<T>(s, "CharacterRig"); }
    template<class T>
    ResourceSelector<T> SkeletonPath(T& s) { return ResourceSelector<T>(s, "Skeleton"); }
    template<class T>
    ResourceSelector<T> SkeletonParamsPath(T& s) { return ResourceSelector<T>(s, "SkeletonParams"); }                   // CHRParams
    template<class T>
    ResourceSelector<T> JointName(T& s) { return ResourceSelector<T>(s, "Joint"); }
    template<class T>
    ResourceSelector<T> AttachmentName(T& s) { return ResourceSelector<T>(s, "Attachment"); }

    // miscelaneous resources
    template<class T>
    ResourceSelector<T> SoundName(T& s) { return ResourceSelector<T>(s, "Sound"); }
    template<class T>
    ResourceSelector<T> DialogName(T& s) { return ResourceSelector<T>(s, "Dialog"); }
    template<class T>
    ResourceSelector<T> ForceFeedbackIdName(T& s) { return ResourceSelector<T>(s, "ForceFeedbackId"); }
    template<class T>
    ResourceSelector<T> ModelFilename(T& s) { return ResourceSelector<T>(s, "Model"); }
    template<class T>
    ResourceSelector<T> ParticleName(T& s) { return ResourceSelector<T>(s, "Particle"); }

    namespace Decorators
    {
        // Decorators namespace is obsolete now, SHOULD NOT BE USED.
        template<class T>
        ResourceSelector<T> AnimationName(T& s) { return ResourceSelector<T>(s, "Animation"); }
        using Serialization::SoundName;
        using Serialization::AttachmentName;
        template<class T>
        ResourceSelector<T> ObjectFilename(T& s) { return ResourceSelector<T>(s, "Model"); }
        using Serialization::JointName;
        using Serialization::ForceFeedbackIdName;
    }
}
#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_RESOURCES_H
