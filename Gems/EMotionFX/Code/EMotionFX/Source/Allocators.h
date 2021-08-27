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

namespace EMotionFX
{
    /*
     * Every time we create a new allocator we need:
     * 1) to create a class that inherits from an another allocator type e.g. AZ::AllocatorBase<AZ::ChildAllocatorSchema<AZ::SystemAllocator>>,  AZ::AllocatorBase<AZ::ChildAllocatorSchema<AZ::PoolAllocator>>
     * 2) that class will contain the UUID
     * 3) we need to create the implementation of the GetDescription method in the cpp file, this is so we don't end up with copies of these strings across compilation units
     * 4) we need to add them to the "Create" method so they are created during component creation
     * 5) during the creation, we need to configure the allocator if it is a sub-allocator
     * 6) we need to add them to the "Destroy" method so they are destroyed during component destruction
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
     * 5) a Description of the allocator
     */

    using SystemAllocatorBase = AZ::SimpleSchemaAllocator<AZ::ChildAllocatorSchema<AZ::SystemAllocator>>;

    // Allocator name,                          Allocator type,      UUID,                                     Description
#define EMOTIONFX_ALLOCATORS \
    ((ActorAllocator)                            (SystemAllocatorBase) ("{7719384C-BC31-4E95-B60C-BA64F5F1D5E9}")("EMotionFX actor memory allocator")) \
    ((AnimGraphAllocator)                        (SystemAllocatorBase) ("{386F92FD-0660-4A4A-8AA8-A748B650279F}")("EMotionFX anim graph memory allocator")) \
    ((CommandAllocator)                          (SystemAllocatorBase) ("{5258FFBC-8E1E-451B-9FD7-073B9C409001}")("EMotionFX commands allocator")) \
    ((GeneralAllocator)                          (SystemAllocatorBase) ("{E259DA95-75DB-4A59-A190-6FB2433D348B}")("EMotionFX general memory allocator")) \
    ((MotionAllocator)                           (SystemAllocatorBase) ("{CAF0B1DB-665F-418B-BEEC-870D8C91C235}")("EMotionFX motion memory allocator")) \
    ((ActorInstanceAllocator)                    (ActorAllocator)     ("{AF2485D0-93B7-4A45-9ACB-A3EFEAAB1746}")("EMotionFX actor instance memory allocator")) \
    ((ActorManagerAllocator)                     (ActorAllocator)     ("{E251E70B-C010-4D21-9521-9A53FE8B9C39}")("EMotionFX actor manager memory allocator")) \
    ((ActorUpdateAllocator)                      (ActorAllocator)     ("{03E10078-F8BC-4F5C-B70B-82B87D15E6B6}")("EMotionFX actor update memory allocator")) \
    ((AnimGraphEventHandlerAllocator)            (AnimGraphAllocator) ("{658DC073-5D33-4E08-B3F2-C9856E29AC9E}")("EMotionFX anim graph event handler memory allocator")) \
    ((AnimGraphGameControllerSettingsAllocator)  (AnimGraphAllocator) ("{8F7CA654-BBDA-4E2D-9916-812AF5A21FBA}")("EMotionFX anim graph controller settings memory allocator")) \
    ((AnimGraphInstanceAllocator)                (AnimGraphAllocator) ("{8C632A30-890C-443C-89F2-F86C6CFD4E15}")("EMotionFX anim graph instance memory allocator")) \
    ((AnimGraphManagerAllocator)                 (AnimGraphAllocator) ("{00A3CA02-55DA-4E21-ADFD-91186CFB4C37}")("EMotionFX anim graph manager memory allocator")) \
    ((AttachmentAllocator)                       (ActorAllocator)     ("{1709A833-ED5C-40BC-B25F-4D61CD148920}")("EMotionFX attachment memory allocator")) \
    ((BlendSpaceManagerAllocator)                (AnimGraphAllocator) ("{505B5C4F-6BB9-49B1-B263-00A4FF9C26E6}")("EMotionFX blend space manager memory allocator")) \
    ((DeformerAllocator)                         (ActorAllocator)     ("{010D01A2-7CB8-4031-ADDB-110503E3AF58}")("EMotionFX deformer memory allocator")) \
    ((EMotionFXManagerAllocator)                 (GeneralAllocator)   ("{C6B6EFAB-61ED-4567-A2D1-6CADBF191E17}")("EMotionFX manager memory allocator")) \
    ((EventManagerAllocator)                     (GeneralAllocator)   ("{69A5606E-5503-4D45-AF21-7CAD707BD7F0}")("EMotionFX event manager memory allocator")) \
    ((EventHandlerAllocator)                     (GeneralAllocator)   ("{ACEBE834-E13E-4E76-A58E-F1CC5C4F8D94}")("EMotionFX event handler memory allocator")) \
    ((EyeBlinkerAllocator)                       (ActorAllocator)     ("{F009290A-E939-4AC8-BFA4-E7D2E51E9396}")("EMotionFX eye blinker memory allocator")) \
    ((ImporterAllocator)                         (GeneralAllocator)   ("{0F6A2BB0-28AC-4AA0-B4A5-ADCB110677B2}")("EMotionFX importer memory allocator")) \
    ((LayerPassAllocator)                        (ActorAllocator)     ("{120D1D14-4E03-4E5A-9B04-7D729EDF8501}")("EMotionFX layer pass memory allocator")) \
    ((MaterialAllocator)                         (ActorAllocator)     ("{E8BD8522-4F4A-467D-9EDA-2C6FC0CFB0BE}")("EMotionFX material memory allocator")) \
    ((MeshAllocator)                             (ActorAllocator)     ("{EC73C463-DBDF-448D-9BD0-AAEB5649E97D}")("EMotionFX mesh memory allocator")) \
    ((MotionEventAllocator)                      (MotionAllocator)    ("{565227EE-E633-4C9D-BA03-1C593D990DB0}")("EMotionFX motion event memory allocator")) \
    ((MotionEventHandlerAllocator)               (MotionAllocator)    ("{658DC073-5D33-4E08-B3F2-C9856E29AC9E}")("EMotionFX motion event handler memory allocator")) \
    ((MotionEventManagerAllocator)               (MotionAllocator)    ("{61D4391F-F86E-404F-959D-0C6085BCD35D}")("EMotionFX motion event manager memory allocator")) \
    ((MotionManagerAllocator)                    (MotionAllocator)    ("{ACFF8545-4D32-42A0-9C1B-6B127029F134}")("EMotionFX motion manager memory allocator")) \
    ((NodeAllocator)                             (ActorAllocator)     ("{52EC041A-2F10-4990-A6C1-5CF43EF49EBC}")("EMotionFX node memory allocator")) \
    ((RecorderAllocator)                         (GeneralAllocator)   ("{E01E8BF7-E103-4E99-AFF4-804D50665C6F}")("EMotionFX recorder memory allocator")) \
    ((SkeletonAllocator)                         (ActorAllocator)     ("{83A8700C-224B-42A1-AB07-4C6E6165D4F4}")("EMotionFX skeleton memory allocator")) \
    ((SoftSkinManagerAllocator)                  (ActorAllocator)     ("{3E70C86F-AC01-475D-9DDC-172287E92F5F}")("EMotionFX soft skin manager memory allocator")) \
    ((ThreadDataAllocator)                       (GeneralAllocator)   ("{E5598A5D-D129-476F-BA46-B316AD491F44}")("EMotionFX thread data memory allocator")) \
    ((TransformDataAllocator)                    (ActorAllocator)     ("{2EFFDE9B-EC69-4F3F-A7F6-F1F47437DF91}")("EMotionFX transform data memory allocator")) \
    ((PoseAllocator)                             (ActorAllocator)     ("{12284635-9AE3-40BD-A0AF-899CE0152352}")("EMotionFX pose memory allocator")) \
    ((EditorAllocator)                           (SystemAllocatorBase)("{7E3FA59C-EFE5-4CFC-959F-153CF8B48605}")("EMotionFX editor allocator")) \

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
#define EMOTIONFX_ALLOCATOR_SEQ_GET_DESCRIPTION(X) EMOTIONFX_SEQ_HEAD_4(X)

#define EMOTIONFX_ALLOCATOR_DECL(ALLOCATOR_SEQUENCE) \
    class EMOTIONFX_ALLOCATOR_SEQ_GET_NAME(ALLOCATOR_SEQUENCE) \
        : public EMOTIONFX_ALLOCATOR_SEQ_GET_TYPE(ALLOCATOR_SEQUENCE) \
    { \
        friend class AZ::AllocatorInstance<EMOTIONFX_ALLOCATOR_SEQ_GET_NAME(ALLOCATOR_SEQUENCE)>; \
    public: \
        AZ_TYPE_INFO(EMOTIONFX_ALLOCATOR_SEQ_GET_NAME(ALLOCATOR_SEQUENCE), EMOTIONFX_ALLOCATOR_SEQ_GET_UUID(ALLOCATOR_SEQUENCE)); \
        using Base = EMOTIONFX_ALLOCATOR_SEQ_GET_TYPE(ALLOCATOR_SEQUENCE); \
        using Descriptor = Base::Descriptor; \
        EMOTIONFX_ALLOCATOR_SEQ_GET_NAME(ALLOCATOR_SEQUENCE)() \
            : Base(TYPEINFO_Name(), EMOTIONFX_ALLOCATOR_SEQ_GET_DESCRIPTION(ALLOCATOR_SEQUENCE)) \
        { \
        } \
        EMOTIONFX_ALLOCATOR_SEQ_GET_NAME(ALLOCATOR_SEQUENCE)(const char* name, const char* desc) \
            : Base(name, desc) \
        { \
        } \
    };

    // Here we create all the classes for all the items in the above table (Step 1)
    AZ_SEQ_FOR_EACH(EMOTIONFX_ALLOCATOR_DECL, EMOTIONFX_ALLOCATORS)

#undef EMOTIONFX_ALLOCATOR_DECL


    // Define the pool allocators
    class AnimGraphConditionAllocator final
        : public AZ::ThreadPoolBase<AnimGraphConditionAllocator>
    {
    public:
        AZ_CLASS_ALLOCATOR(AnimGraphConditionAllocator, AZ::SystemAllocator, 0)
        AZ_TYPE_INFO(AnimGraphConditionAllocator, "{F5406A89-3F11-4791-9F83-6A71D0F8DD81}")

        using Base = AZ::ThreadPoolBase<AnimGraphConditionAllocator>;

        AnimGraphConditionAllocator()
        : Base("AnimGraphConditionAllocator", "EMotionFX anim graph condition memory allocator")
        {
        }
    };


    class AnimGraphObjectDataAllocator final
        : public AZ::ThreadPoolBase<AnimGraphObjectDataAllocator>
    {
    public:
        AZ_CLASS_ALLOCATOR(AnimGraphObjectDataAllocator, AZ::SystemAllocator, 0)
        AZ_TYPE_INFO(AnimGraphObjectDataAllocator, "{E00ADC25-A311-4003-849E-85C125089C74}")

        using Base = AZ::ThreadPoolBase<AnimGraphObjectDataAllocator>;

        AnimGraphObjectDataAllocator()
        : Base("AnimGraphObjectDataAllocator", "EMotionFX anim graph object data memory allocator")
        {
        }
    };


    class AnimGraphObjectUniqueDataAllocator final
        : public AZ::ThreadPoolBase<AnimGraphObjectUniqueDataAllocator>
    {
    public:
        AZ_CLASS_ALLOCATOR(AnimGraphObjectUniqueDataAllocator, AZ::SystemAllocator, 0)
            AZ_TYPE_INFO(AnimGraphObjectUniqueDataAllocator, "{C74F51E0-E6B0-4EF8-A3BF-0968CAEF1333}")

        using Base = AZ::ThreadPoolBase<AnimGraphObjectUniqueDataAllocator>;

        AnimGraphObjectUniqueDataAllocator()
        : Base("AnimGraphObjectUniqueDataAllocator", "EMotionFX anim graph object unique data memory allocator")
        {
        }
    };

    class Allocators
    {
    public:
        static void Create();
        static void Destroy();

        static void ShrinkPools();
    };

} // namespace EMotionFX
