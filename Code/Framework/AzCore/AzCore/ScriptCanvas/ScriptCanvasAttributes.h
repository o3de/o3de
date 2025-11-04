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
            AZ_CLASS_ALLOCATOR(GetUnpackedReturnValueTypesHolder, AZ::SystemAllocator);

            GetUnpackedReturnValueTypes m_function;
        };

        using TupleConstructor = void*(*)();
        struct TupleConstructorHolder
        {
            AZ_TYPE_INFO(TupleConstructorHolder, "{E7BCBB72-E3F7-4E8B-BBA1-7824603650DB}");
            AZ_CLASS_ALLOCATOR(TupleConstructorHolder, AZ::SystemAllocator);

            TupleConstructor m_function;
        };

        ///< Marks a class for internal, usually unserialized, use
        static const AZ::Crc32 AllowInternalCreation = AZ_CRC_CE("AllowInternalCreation");

        ///< Marks a node as an action that deactivates self, and does so in a way that is detectable at compile time
        static const AZ::Crc32 DeactivatesInputEntity = AZ_CRC_CE("DeactivatesInputEntity");

        ///< This is used to provide a more readable name for ScriptCanvas nodes
        static const AZ::Crc32 PrettyName = AZ_CRC_CE("PrettyName");
        ///< This is used to forbid variable creation of types in ScriptCanvas nodes
        static const AZ::Crc32 VariableCreationForbidden = AZ_CRC_CE("VariableCreationForbidden");
        ///< Used to mark a function as a tuple retrieval function. The value for Index is what index is used to retrive the function
        static const AZ::Crc32 TupleGetFunctionIndex = AZ_CRC_CE("TupleGetFunction");

        ///< Used to unpack the types of a single return type that can be unpacked, eg AZStd::tuple or AZ::Outcome
        static const AZ::Crc32 ReturnValueTypesFunction = AZ_CRC_CE("ReturnValueTypesFunction");

        ///< Used to construct zero-initialized a tuple type
        static const AZ::Crc32 TupleConstructorFunction = AZ_CRC_CE("TupleConstructorFunction");

        ///< Used to mark a function as a floating function separate from whatever class reflected it.
        static const AZ::Crc32 FloatingFunction = AZ_CRC_CE("FloatingFunction");

        ///< Used to mark a slot as hidden from the SC author at edit time
        using HiddenIndices = AZStd::vector<size_t>;
        static const AZ::Crc32 HiddenParameterIndex = AZ_CRC_CE("HiddenParameterIndex");

        ///<
        static const AZ::Crc32 ExplicitOverloadCrc = AZ_CRC_CE("ExplicitOverload");

        ///< Used to mark overload method argument dynamic group for ScriptCanvas node
        static const AZ::Crc32 OverloadArgumentGroup = AZ_CRC_CE("OverloadArgumentGroup");

        ///<
        static const AZ::Crc32 CheckedOperation = AZ_CRC_CE("CheckedOperation");

        static const AZ::Crc32 BranchOnResult = AZ_CRC_CE("BranchOnResult");

        ///< \see enum class Script::Attributes::OperatorType
        static const AZ::Crc32 OperatorOverride = AZ_CRC_CE("OperatorOverride");

        namespace Internal
        {
            ///< Implemented as a Node generic function
            static const AZ::Crc32 ImplementedAsNodeGeneric = AZ_CRC_CE("ImplementedAsNodeGeneric");
        }

        ///< If that output of a function is an AZ::Outcome, unpack elements into separate slots
        static const AZ::Crc32 AutoUnpackOutputOutcomeSlots = AZ_CRC_CE("AutoUnpackOutputOutcomeSlots");
        ///< If that output of a function is an AZ::Outcome, unpack elements into separate slots
        static const AZ::Crc32 AutoUnpackOutputOutcomeFailureSlotName= AZ_CRC_CE("AutoUnpackOutputOutcomeFailureSlotName");
        ///< If that output of a function is an AZ::Outcome, unpack elements into separate slots
        static const AZ::Crc32 AutoUnpackOutputOutcomeSuccessSlotName = AZ_CRC_CE("AutoUnpackOutputOutcomeSuccessSlotName");

    }
}
