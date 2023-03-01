/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// Forward declare common classes and structs that allows the references
// to the SerializeContext without any includes
namespace AZ
{
    template<class T, class>
    struct AnyTypeInfoConcept;

    class GenericClassInfo;
    class ReflectContext;
    class SerializeContext;
}

namespace AZ::Serialize
{
    template<class, bool, bool>
    struct InstanceFactory;

    class ClassData;
    struct EnumerateInstanceCallContext;
    struct ClassElement;
    struct DataElement;
    class DataElementNode;
    class IObjectFactory;
    class IDataSerializer;
    class IDataContainer;
    class IEventHandler;
    class IDataConverter;
}

// Put this macro on your class to allow serialization with a private default constructor.
#define AZ_SERIALIZE_FRIEND() \
    template <typename, typename> \
    friend struct AZ::AnyTypeInfoConcept; \
    template <typename, bool, bool> \
    friend struct AZ::Serialize::InstanceFactory;
