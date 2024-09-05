/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/RTTI/ReflectContext.h>
#include "EMotionFXConfig.h"
#include <MCore/Source/Stream.h>
#include <MCore/Source/CommandLine.h>
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/Attribute.h>
#include <MCore/Source/AttributeFloat.h>
#include <MCore/Source/AttributeInt32.h>
#include <MCore/Source/AttributeString.h>
#include <MCore/Source/AttributeBool.h>
#include <MCore/Source/AttributeVector2.h>
#include <MCore/Source/AttributeVector3.h>
#include <MCore/Source/AttributeVector4.h>
#include <MCore/Source/AttributeQuaternion.h>
#include <MCore/Source/AttributeColor.h>
#include "AnimGraphObjectData.h"
#include "Transform.h"


namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;
    class AnimGraph;
    class MotionSet;
    class AnimGraphNode;
    class AnimGraphInstance;
    class AnimGraphRefCountedData;

    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphObject
    {
    public:
        AZ_RTTI(AnimGraphObject, "{532F5328-9AE3-4793-A7AA-8DEB0BAC9A9E}")
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphObject();
        AnimGraphObject(AnimGraph* animGraph);
        virtual ~AnimGraphObject();

        enum
        {
            FLAG_DISABLED       = 1 << 0    // if the attribute is disabled
        };

        enum ECategory
        {
            CATEGORY_SOURCES                = 0,
            CATEGORY_BLENDING               = 1,
            CATEGORY_CONTROLLERS            = 2,
            CATEGORY_PHYSICS                = 3,
            CATEGORY_LOGIC                  = 4,
            CATEGORY_MATH                   = 5,
            CATEGORY_MISC                   = 6,
            CATEGORY_TRANSITIONS            = 10,
            CATEGORY_TRANSITIONCONDITIONS   = 11,
            CATEGORY_TRIGGERACTIONS         = 12
        };
        static const char* GetCategoryName(ECategory category);

        enum ESyncMode : AZ::u8
        {
            SYNCMODE_DISABLED               = 0,
            SYNCMODE_TRACKBASED             = 1,
            SYNCMODE_CLIPBASED              = 2
        };

        enum EEventMode : AZ::u8
        {
            EVENTMODE_LEADERONLY            = 0,
            EVENTMODE_FOLLOWERONLY          = 1,
            EVENTMODE_BOTHNODES             = 2,
            EVENTMODE_MOSTACTIVE            = 3,
            EVENTMODE_NONE                  = 4
        };

        enum EExtractionMode : AZ::u8
        {
            EXTRACTIONMODE_BLEND            = 0,
            EXTRACTIONMODE_TARGETONLY       = 1,
            EXTRACTIONMODE_SOURCEONLY       = 2
        };

        /**
         * Reinitialize the object.
         * Some anim graph objects might have additional member variables which are not reflected. These are mostly used for optimizations, e.g. a condition that stores a parameter name which is reflected
         * but the runtime uses a cached parameter index to prevent runtime lookups. These cached values need to be updated on given events like when e.g. a parameter gets removed or changed or the whole
         * anim graph object gets constructed by a copy and paste operation.
         */
        virtual void Reinit();

        virtual void RecursiveReinit();

        virtual AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) { return aznew AnimGraphObjectData(this, animGraphInstance); }

        /// Calls InvalidateUniqueData() for the given object for all anim graph instances. (Used by reflection context)
        void InvalidateUniqueDatas();

        // Invalidates given object including all owned objects that do not represent a hierarchy, e.g. transition invalidate conditions, nodes invalidate actions.
        // This will only invalidate already created unique datas and skip e.g. to invalidate unique datas for non yet reached nodes.
        virtual void InvalidateUniqueData(AnimGraphInstance* animGraphInstance);
        virtual void RecursiveInvalidateUniqueDatas(AnimGraphInstance* animGraphInstance) { InvalidateUniqueData(animGraphInstance); }

        void ResetUniqueDatas();
        void ResetUniqueData(AnimGraphInstance* animGraphInstance);

        virtual bool InitAfterLoading(AnimGraph* animGraph) = 0;

        virtual void RegisterAttributes() {}
        virtual void Unregister();
        virtual const char* GetPaletteName() const = 0;
        virtual void GetSummary(AZStd::string* outResult) const;
        virtual void GetTooltip(AZStd::string* outResult) const;
        virtual const char* GetHelpUrl() const;
        virtual ECategory GetPaletteCategory() const = 0;

        void InitInternalAttributesForAllInstances();   // does the init for all anim graph instances in the parent animgraph
        virtual void InitInternalAttributes(AnimGraphInstance* animGraphInstance);
        virtual void RemoveInternalAttributesForAllInstances();
        virtual void DecreaseInternalAttributeIndices(size_t decreaseEverythingHigherThan);

        virtual void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds);


        virtual void OnChangeMotionSet(AnimGraphInstance* animGraphInstance, MotionSet* newMotionSet)            { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(newMotionSet); }
        virtual void OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)                             { MCORE_UNUSED(animGraph);         MCORE_UNUSED(nodeToRemove); }
        virtual void RecursiveOnChangeMotionSet(AnimGraphInstance* animGraphInstance, MotionSet* newMotionSet)   { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(newMotionSet); }
        virtual void OnActorMotionExtractionNodeChanged()                                                        {}

        MCORE_INLINE size_t GetObjectIndex() const                                      { return m_objectIndex; }
        MCORE_INLINE void SetObjectIndex(size_t index)                                  { m_objectIndex = index; }

        MCORE_INLINE AnimGraph* GetAnimGraph() const                                    { return m_animGraph; }
        MCORE_INLINE void SetAnimGraph(AnimGraph* animGraph)                            { m_animGraph = animGraph; }

        size_t SaveUniqueData(AnimGraphInstance* animGraphInstance, uint8* outputBuffer) const;  // save and return number of bytes written, when outputBuffer is nullptr only return num bytes it would write
        size_t LoadUniqueData(AnimGraphInstance* animGraphInstance, const uint8* dataBuffer);    // load and return number of bytes read, when dataBuffer is nullptr, 0 should be returned

        virtual void RecursiveCollectObjects(AZStd::vector<AnimGraphObject*>& outObjects) const;

        bool GetHasErrorFlag(AnimGraphInstance* animGraphInstance) const;
        void SetHasErrorFlag(AnimGraphInstance* animGraphInstance, bool hasError);

        void SyncVisualObject();

        static void CalculateMotionExtractionDelta(EExtractionMode extractionMode, AnimGraphRefCountedData* sourceRefData, AnimGraphRefCountedData* targetRefData, float weight, bool hasMotionExtractionNodeInMask, Transform& outTransform, Transform& outTransformMirrored);
        static void CalculateMotionExtractionDeltaAdditive(EExtractionMode extractionMode, AnimGraphRefCountedData* sourceRefData, AnimGraphRefCountedData* targetRefData, const Transform& basePoseTransform, float weight, bool hasMotionExtractionNodeInMask, Transform& outTransform, Transform& outTransformMirrored);

        static void Reflect(AZ::ReflectContext* context);

    protected:
        AnimGraph*                          m_animGraph;
        size_t                              m_objectIndex;
    };
} // namespace EMotionFX


namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::AnimGraphObject::ESyncMode, "{55457918-FC3A-4344-A524-EC70E052239D}");
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::AnimGraphObject::EEventMode, "{DE3845CA-ECA6-4359-999D-6760D6D8C249}");
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::AnimGraphObject::EExtractionMode, "{E93850ED-6CC1-45B0-AA75-BDBDAE259F79}");
} // namespace AZ
