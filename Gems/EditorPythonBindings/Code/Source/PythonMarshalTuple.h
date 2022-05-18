/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Source/PythonMarshalComponent.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace EditorPythonBindings
{
    class TypeConverterTuple final : public PythonMarshalComponent::TypeConverter
    {
    public:
        TypeConverterTuple(
            [[maybe_unused]] AZ::GenericClassInfo* genericClassInfo,
            const AZ::SerializeContext::ClassData* classData,
            const AZ::TypeId& typeId)
            : m_classData(classData)
            , m_typeId(typeId)
        {
        }

        AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> PythonToBehaviorValueParameter(
            PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue) override;

        AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> BehaviorValueParameterToPython(
            AZ::BehaviorArgument& behaviorValue) override;

        bool CanConvertPythonToBehaviorValue(PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj) const override;

    protected:
        const AZ::SerializeContext::ClassData* m_classData = nullptr;
        const AZ::TypeId m_typeId = {};

        bool IsValidList(pybind11::object pyObj) const;
        bool IsValidTuple(pybind11::object pyObj) const;
        bool IsCompatibleProxy(pybind11::object pyObj) const;

        static bool LoadPythonToTupleElement(
            PyObject* pyItem,
            PythonMarshalTypeRequests::BehaviorTraits traits,
            const AZ::SerializeContext::ClassElement* itemElement,
            AZ::SerializeContext::IDataContainer* tupleContainer,
            size_t index,
            AZ::SerializeContext* serializeContext,
            void* newTuple);
    };
} // namespace EditorPythonBindings
