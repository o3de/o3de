/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Preprocessor/Sequences.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Memory/AllocatorBase.h>
#include <AzCore/Memory/ChildAllocatorSchema.h>

namespace EMotionFX
{
    /*
     * Every time we create a new allocator we need:
     * 1) to create a class that inherits from an another allocator type e.g. AZ::AllocatorBase<AZ::ChildAllocatorSchema<AZ::SystemAllocator>>,  AZ::AllocatorBase<AZ::ChildAllocatorSchema<AZ::PoolAllocator>>
     * 2) that class will contain the UUID
     *
     * Forgetting one of this will either cause a compilation error or a runtime error on the first allocation with that allocator (which is sometimes hard to track)
     * To avoid forgetting anything and to help the developer that wants to add/remove a new allocator, we built this table and a set of macros that expand into the above steps
     * To add/remove allocators we don't need to touch anything from the below macros, just this table.
     * The table is filled with the following information:
     * 1) the allocator name, this will be the name used in the class(es) that want to use such allocator
     * 2) the base allocator type: the allocator they inherit from. This is the type of memory allocation this allocator is going to use.
     *    If no base allocator, use NoSubAllocator.
     * 3) If this is a sub-allocator of another allocator, this entry will contain which. If it doesn't, then this allocator is its own allocator
     * 4) the UUID, required by the RTTI system, defines a unique type identifier for this allocator
     */

    using SystemAllocatorBase = AZ::ChildAllocatorSchema<AZ::SystemAllocator>;

    // Allocator name,                          Allocator type,      UUID
#define EMOTIONFX_ALLOCATORS \
    ((ActorAllocator)                            (SystemAllocatorBase) ("{7719384C-BC31-4E95-B60C-BA64F5F1D5E9}")) \
    ((AnimGraphAllocator)                        (SystemAllocatorBase) ("{386F92FD-0660-4A4A-8AA8-A748B650279F}")) \
    ((CommandAllocator)                          (SystemAllocatorBase) ("{5258FFBC-8E1E-451B-9FD7-073B9C409001}")) \
    ((GeneralAllocator)                          (SystemAllocatorBase) ("{E259DA95-75DB-4A59-A190-6FB2433D348B}")) \
    ((MotionAllocator)                           (SystemAllocatorBase) ("{CAF0B1DB-665F-418B-BEEC-870D8C91C235}")) \
    ((ActorInstanceAllocator)                    (ActorAllocator)     ("{AF2485D0-93B7-4A45-9ACB-A3EFEAAB1746}")) \
    ((ActorManagerAllocator)                     (ActorAllocator)     ("{E251E70B-C010-4D21-9521-9A53FE8B9C39}")) \
    ((ActorUpdateAllocator)                      (ActorAllocator)     ("{03E10078-F8BC-4F5C-B70B-82B87D15E6B6}")) \
    ((AnimGraphEventHandlerAllocator)            (AnimGraphAllocator) ("{658DC073-5D33-4E08-B3F2-C9856E29AC9E}")) \
    ((AnimGraphInstanceAllocator)                (AnimGraphAllocator) ("{8C632A30-890C-443C-89F2-F86C6CFD4E15}")) \
    ((AnimGraphManagerAllocator)                 (AnimGraphAllocator) ("{00A3CA02-55DA-4E21-ADFD-91186CFB4C37}")) \
    ((AttachmentAllocator)                       (ActorAllocator)     ("{1709A833-ED5C-40BC-B25F-4D61CD148920}")) \
    ((BlendSpaceManagerAllocator)                (AnimGraphAllocator) ("{505B5C4F-6BB9-49B1-B263-00A4FF9C26E6}")) \
    ((DeformerAllocator)                         (ActorAllocator)     ("{010D01A2-7CB8-4031-ADDB-110503E3AF58}")) \
    ((EMotionFXManagerAllocator)                 (GeneralAllocator)   ("{C6B6EFAB-61ED-4567-A2D1-6CADBF191E17}")) \
    ((EventManagerAllocator)                     (GeneralAllocator)   ("{69A5606E-5503-4D45-AF21-7CAD707BD7F0}")) \
    ((EventHandlerAllocator)                     (GeneralAllocator)   ("{ACEBE834-E13E-4E76-A58E-F1CC5C4F8D94}")) \
    ((EyeBlinkerAllocator)                       (ActorAllocator)     ("{F009290A-E939-4AC8-BFA4-E7D2E51E9396}")) \
    ((ImporterAllocator)                         (GeneralAllocator)   ("{0F6A2BB0-28AC-4AA0-B4A5-ADCB110677B2}")) \
    ((LayerPassAllocator)                        (ActorAllocator)     ("{120D1D14-4E03-4E5A-9B04-7D729EDF8501}")) \
    ((MaterialAllocator)                         (ActorAllocator)     ("{E8BD8522-4F4A-467D-9EDA-2C6FC0CFB0BE}")) \
    ((MeshAllocator)                             (ActorAllocator)     ("{EC73C463-DBDF-448D-9BD0-AAEB5649E97D}")) \
    ((MotionEventAllocator)                      (MotionAllocator)    ("{565227EE-E633-4C9D-BA03-1C593D990DB0}")) \
    ((MotionEventHandlerAllocator)               (MotionAllocator)    ("{658DC073-5D33-4E08-B3F2-C9856E29AC9E}")) \
    ((MotionEventManagerAllocator)               (MotionAllocator)    ("{61D4391F-F86E-404F-959D-0C6085BCD35D}")) \
    ((MotionManagerAllocator)                    (MotionAllocator)    ("{ACFF8545-4D32-42A0-9C1B-6B127029F134}")) \
    ((NodeAllocator)                             (ActorAllocator)     ("{52EC041A-2F10-4990-A6C1-5CF43EF49EBC}")) \
    ((RecorderAllocator)                         (GeneralAllocator)   ("{E01E8BF7-E103-4E99-AFF4-804D50665C6F}")) \
    ((SkeletonAllocator)                         (ActorAllocator)     ("{83A8700C-224B-42A1-AB07-4C6E6165D4F4}")) \
    ((SoftSkinManagerAllocator)                  (ActorAllocator)     ("{3E70C86F-AC01-475D-9DDC-172287E92F5F}")) \
    ((ThreadDataAllocator)                       (GeneralAllocator)   ("{E5598A5D-D129-476F-BA46-B316AD491F44}")) \
    ((TransformDataAllocator)                    (ActorAllocator)     ("{2EFFDE9B-EC69-4F3F-A7F6-F1F47437DF91}")) \
    ((PoseAllocator)                             (ActorAllocator)     ("{12284635-9AE3-40BD-A0AF-899CE0152352}")) \
    ((EditorAllocator)                           (SystemAllocatorBase)("{7E3FA59C-EFE5-4CFC-959F-153CF8B48605}")) \

    // if it exceeds 50, needs adjustments in AzCore/Preprocessor/Sequences.h

// Strips from the HEAD
#define EMOTIONFX_SEQ_TAIL_STRIP_1(X) AZ_SEQ_TAIL(X)
#define EMOTIONFX_SEQ_TAIL_STRIP_2(X) EMOTIONFX_SEQ_TAIL_STRIP_1(AZ_SEQ_TAIL(X))
#define EMOTIONFX_SEQ_TAIL_STRIP_3(X) EMOTIONFX_SEQ_TAIL_STRIP_2(AZ_SEQ_TAIL(X))

// Gets the N element from the head
#define EMOTIONFX_SEQ_HEAD_1(X) AZ_SEQ_HEAD(X)
#define EMOTIONFX_SEQ_HEAD_2(X) EMOTIONFX_SEQ_HEAD_1(EMOTIONFX_SEQ_TAIL_STRIP_1(X))
#define EMOTIONFX_SEQ_HEAD_3(X) EMOTIONFX_SEQ_HEAD_1(EMOTIONFX_SEQ_TAIL_STRIP_2(X))
#define EMOTIONFX_SEQ_HEAD_4(X) EMOTIONFX_SEQ_HEAD_1(EMOTIONFX_SEQ_TAIL_STRIP_3(X))

// Gets specific arguments so the macros are more readable
#define EMOTIONFX_ALLOCATOR_SEQ_GET_NAME(X) EMOTIONFX_SEQ_HEAD_1(X)
#define EMOTIONFX_ALLOCATOR_SEQ_GET_TYPE(X) EMOTIONFX_SEQ_HEAD_2(X)
#define EMOTIONFX_ALLOCATOR_SEQ_GET_UUID(X) EMOTIONFX_SEQ_HEAD_3(X)

// Helper macro which allows the EMOTIONFX_ALLOCATOR_SEQ_GET_* macros
// to only need to expand a single time as a conformant C preprocessor will
// expand the same macro call multiple times in a single macro invocation
// i.e `EMOTIONFX_ALLOCATOR_SEQ_GET_NAME(ALLOCATOR_SEQUENCE)` will only get
// expanded the first time it processed in a macro
// If it is repeated multiple times, it will be left unexpanded
#define EMOTIONFX_ALLOCATOR_DECL_EXPANDER(_ALLOCATOR_NAME, _ALLOCATOR_UUID, _ALLOCATOR_BASE) \
    class _ALLOCATOR_NAME \
        : public _ALLOCATOR_BASE \
    { \
        friend class AZ::AllocatorInstance<_ALLOCATOR_NAME>; \
    public: \
        AZ_TYPE_INFO(_ALLOCATOR_NAME, _ALLOCATOR_UUID); \
        using Base = _ALLOCATOR_BASE; \
        AZ_RTTI_NO_TYPE_INFO_DECL(); \
    };\
    \
    AZ_RTTI_NO_TYPE_INFO_IMPL_INLINE( \
        _ALLOCATOR_NAME, _ALLOCATOR_NAME ::Base);


#define EMOTIONFX_ALLOCATOR_DECL(ALLOCATOR_SEQUENCE) \
    EMOTIONFX_ALLOCATOR_DECL_EXPANDER( \
        EMOTIONFX_ALLOCATOR_SEQ_GET_NAME(ALLOCATOR_SEQUENCE), \
        EMOTIONFX_ALLOCATOR_SEQ_GET_UUID(ALLOCATOR_SEQUENCE), \
        EMOTIONFX_ALLOCATOR_SEQ_GET_TYPE(ALLOCATOR_SEQUENCE))

    // Here we create all the classes for all the items in the above table (Step 1)
    AZ_SEQ_FOR_EACH(EMOTIONFX_ALLOCATOR_DECL, EMOTIONFX_ALLOCATORS)

#undef EMOTIONFX_ALLOCATOR_DECL


    // Define the pool allocators
    class AnimGraphConditionAllocator;
}
namespace AZ::Internal
{
    // extern the threadsafe PoolAllocator schema for the graph condition allocator
    extern template class PoolAllocatorHelper<ThreadPoolSchemaHelper<EMotionFX::AnimGraphConditionAllocator>>;
}
namespace EMotionFX
{
    class AnimGraphConditionAllocator final
        : public AZ::ThreadPoolBase<AnimGraphConditionAllocator>
    {
        using Base = AZ::ThreadPoolBase<AnimGraphConditionAllocator>;
    public:
        AZ_CLASS_ALLOCATOR(AnimGraphConditionAllocator, AZ::SystemAllocator);
        AZ_RTTI(AnimGraphConditionAllocator, "{F5406A89-3F11-4791-9F83-6A71D0F8DD81}", Base);
    };


    class AnimGraphObjectDataAllocator;
}
namespace AZ::Internal
{
    // extern the threadsafe PoolAllocator schema for the graph object data allocator
    extern template class PoolAllocatorHelper<ThreadPoolSchemaHelper<EMotionFX::AnimGraphObjectDataAllocator>>;
}
namespace EMotionFX
{
    class AnimGraphObjectDataAllocator final
        : public AZ::ThreadPoolBase<AnimGraphObjectDataAllocator>
    {
        using Base = AZ::ThreadPoolBase<AnimGraphObjectDataAllocator>;
    public:
        AZ_CLASS_ALLOCATOR(AnimGraphObjectDataAllocator, AZ::SystemAllocator);
        AZ_RTTI(AnimGraphObjectDataAllocator, "{E00ADC25-A311-4003-849E-85C125089C74}", Base);
    };


    class AnimGraphObjectUniqueDataAllocator;
}
namespace AZ::Internal
{
    // extern the threadsafe PoolAllocator schema for the graph object unique data allocator
    extern template class PoolAllocatorHelper<ThreadPoolSchemaHelper<EMotionFX::AnimGraphObjectUniqueDataAllocator>>;
}
namespace EMotionFX
{
    class AnimGraphObjectUniqueDataAllocator final
        : public AZ::ThreadPoolBase<AnimGraphObjectUniqueDataAllocator>
    {
        using Base = AZ::ThreadPoolBase<AnimGraphObjectUniqueDataAllocator>;
    public:
        AZ_CLASS_ALLOCATOR(AnimGraphObjectUniqueDataAllocator, AZ::SystemAllocator);
        AZ_RTTI(AnimGraphObjectUniqueDataAllocator, "{C74F51E0-E6B0-4EF8-A3BF-0968CAEF1333}", Base);

    };

    class Allocators
    {
    public:
        static void ShrinkPools();
    };

} // namespace EMotionFX
