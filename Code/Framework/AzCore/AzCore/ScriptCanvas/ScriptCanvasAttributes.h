/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Crc.h>

namespace AZ
{
    namespace ScriptCanvasAttributes
    {
        using GetUnpackedReturnValueTypes = AZStd::vector<AZ::TypeId>(*)();
        struct GetUnpackedReturnValueTypesHolder
        {
            AZ_TYPE_INFO(GetUnpackedReturnValueTypesHolder, "{DCE581D4-5F89-4A7A-BA67-9A7B13B580C2}");
            AZ_CLASS_ALLOCATOR(GetUnpackedReturnValueTypesHolder, AZ::SystemAllocator, 0);

            GetUnpackedReturnValueTypes m_function;
        };

        using TupleConstructor = void*(*)();
        struct TupleConstructorHolder
        {
            AZ_TYPE_INFO(TupleConstructorHolder, "{E7BCBB72-E3F7-4E8B-BBA1-7824603650DB}");
            AZ_CLASS_ALLOCATOR(TupleConstructorHolder, AZ::SystemAllocator, 0);

            TupleConstructor m_function;
        };

        ///< Marks a class for internal, usually unserialized, use
        static const AZ::Crc32 AllowInternalCreation = AZ_CRC("AllowInternalCreation", 0x4817d9fb);

        ///< Marks a node as an action that deactivates self, and does so in a way that is detectable at compile time
        static const AZ::Crc32 DeactivatesInputEntity = AZ_CRC_CE("DeactivatesInputEntity");

        ///< This is used to provide a more readable name for ScriptCanvas nodes
        static const AZ::Crc32 PrettyName = AZ_CRC("PrettyName", 0x50ac3029);
        ///< This is used to forbid variable creation of types in ScriptCanvas nodes
        static const AZ::Crc32 VariableCreationForbidden = AZ_CRC("VariableCreationForbidden", 0x0034f5bb);
        ///< Used to mark a function as a tuple retrieval function. The value for Index is what index is used to retrive the function
        static const AZ::Crc32 TupleGetFunctionIndex = AZ_CRC("TupleGetFunction", 0x50020c16);

        ///< Used to unpack the types of a single return type that can be unpacked, eg AZStd::tuple or AZ::Outcome
        static const AZ::Crc32 ReturnValueTypesFunction = AZ_CRC("ReturnValueTypesFunction", 0x557e7ed1);

        ///< Used to construct zero-initialized a tuple type
        static const AZ::Crc32 TupleConstructorFunction = AZ_CRC("TupleConstructorFunction", 0x36a77490);

        ///< Used to mark a function as a floating function separate from whatever class reflected it.
        static const AZ::Crc32 FloatingFunction = AZ_CRC("FloatingFunction", 0xdcf978f9);

        ///< Used to mark a slot as hidden from the SC author at edit time
        using HiddenIndices = AZStd::vector<size_t>;
        static const AZ::Crc32 HiddenParameterIndex = AZ_CRC("HiddenParameterIndex", 0xabfdb58b);

        ///<
        static const AZ::Crc32 ExplicitOverloadCrc = AZ_CRC("ExplicitOverload", 0xbd3a878d);

        ///< Used to mark overload method argument dynamic group for ScriptCanvas node
        static const AZ::Crc32 OverloadArgumentGroup = AZ_CRC("OverloadArgumentGroup", 0x24e595bb);

        ///<
        static const AZ::Crc32 CheckedOperation = AZ_CRC("CheckedOperation", 0x95054825);

        static const AZ::Crc32 BranchOnResult = AZ_CRC("BranchOnResult", 0x7a741f05);

        ///< \see enum class Script::Attributes::OperatorType
        static const AZ::Crc32 OperatorOverride = AZ_CRC("OperatorOverride", 0x6b0e3ffb);

        namespace Internal
        {
            ///< Implemented as a Node generic function
            static const AZ::Crc32 ImplementedAsNodeGeneric = AZ_CRC_CE("ImplementedAsNodeGeneric");
        }

        ///< If that output of a function is an AZ::Outcome, unpack elements into separate slots
        static const AZ::Crc32 AutoUnpackOutputOutcomeSlots = AZ_CRC("AutoUnpackOutputOutcomeSlots", 0x2664162d);
        ///< If that output of a function is an AZ::Outcome, unpack elements into separate slots
        static const AZ::Crc32 AutoUnpackOutputOutcomeFailureSlotName= AZ_CRC("AutoUnpackOutputOutcomeFailureSlotName", 0xfc72dd3f);
        ///< If that output of a function is an AZ::Outcome, unpack elements into separate slots
        static const AZ::Crc32 AutoUnpackOutputOutcomeSuccessSlotName = AZ_CRC("AutoUnpackOutputOutcomeSuccessSlotName", 0x22ac22d5);

    }
}
