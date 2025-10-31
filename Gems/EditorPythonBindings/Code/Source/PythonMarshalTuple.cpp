/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/PythonMarshalTuple.h>
#include <Source/PythonProxyObject.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <pybind11/embed.h>
#include <pybind11/pytypes.h>

namespace EditorPythonBindings
{
    // Check to see if the input object is a valid Python list.
    bool TypeConverterTuple::IsValidList(pybind11::object pyObj) const
    {
        return PyList_Check(pyObj.ptr()) != false;
    }

    // Check to see if the input object is a valid Python tuple.
    bool TypeConverterTuple::IsValidTuple(pybind11::object pyObj) const
    {
        return PyTuple_Check(pyObj.ptr()) != false;
    }

    // Check to see if the input object is a valid Python proxy object of a C++ tuple.
    bool TypeConverterTuple::IsCompatibleProxy(pybind11::object pyObj) const
    {
        if (pybind11::isinstance<EditorPythonBindings::PythonProxyObject>(pyObj))
        {
            auto behaviorObject = pybind11::cast<EditorPythonBindings::PythonProxyObject*>(pyObj)->GetBehaviorObject();
            AZ::Uuid typeId = behaviorObject.value()->m_typeId;

            return AZ::Utils::IsTupleContainerType(typeId);
        }

        return false;
    }

    // If the input object is either a Python list, Python tuple, or Proxy object of a C++ tuple, it can be converted
    // (or at least attempted to be converted) to a C++ tuple type.
    bool TypeConverterTuple::CanConvertPythonToBehaviorValue(
        [[maybe_unused]] PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj) const
    {
        return IsValidList(pyObj) || IsValidTuple(pyObj) || IsCompatibleProxy(pyObj);
    }

    // Given a Python object, clone it into a specific element in the tuple.
    bool TypeConverterTuple::LoadPythonToTupleElement(
        PyObject* pyItem,
        PythonMarshalTypeRequests::BehaviorTraits traits,
        const AZ::SerializeContext::ClassElement* itemElement,
        AZ::SerializeContext::IDataContainer* tupleContainer,
        size_t index,
        AZ::SerializeContext* serializeContext,
        void* newTuple)
    {
        pybind11::object pyObj{ pybind11::reinterpret_borrow<pybind11::object>(pyItem) };
        AZ::BehaviorArgument behaviorItem;
        auto behaviorResult = Container::ProcessPythonObject(traits, pyObj, itemElement->m_typeId, behaviorItem);
        if (behaviorResult && behaviorResult.value().first)
        {
            void* itemAddress = tupleContainer->GetElementByIndex(newTuple, itemElement, index);
            if (!itemAddress)
            {
                AZ_Error(
                    "python", itemAddress,
                    "Element reserved for associative container's tuple, but unable to retrieve address of the item:%d", index);
                return false;
            }
            serializeContext->CloneObjectInplace(itemAddress, behaviorItem.m_value, itemElement->m_typeId);
        }
        else
        {
            AZ_Warning(
                "python", false, "Could not convert to tuple element type %s for the tuple<>; failed to marshal Python input %s",
                itemElement->m_name, Convert::GetPythonTypeName(pyObj).c_str());
            return false;
        }
        return true;
    }

    // Convert a Python list / Python tuple / ProxyObject tuple to a C++ tuple.
    AZStd::optional<PythonMarshalTypeRequests::BehaviorValueResult> TypeConverterTuple::PythonToBehaviorValueParameter(
        PythonMarshalTypeRequests::BehaviorTraits traits, pybind11::object pyObj, AZ::BehaviorArgument& outValue)
    {
        if (!CanConvertPythonToBehaviorValue(traits, pyObj))
        {
            AZ_Warning("python", false, "Cannot convert tuple container for %s", m_classData->m_name);
            return AZStd::nullopt;
        }

        const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(m_typeId);
        if (!behaviorClass)
        {
            AZ_Warning("python", false, "Missing tuple behavior class for %s", m_typeId.ToString<AZStd::string>().c_str());
            return AZStd::nullopt;
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        if (!serializeContext)
        {
            return AZStd::nullopt;
        }

        // prepare the AZStd::tuple<> container
        AZ::BehaviorObject tupleInstance = behaviorClass->Create();
        AZ::SerializeContext::IDataContainer* tupleDataContainer = m_classData->m_container;

        // get the element types
        AZStd::vector<const AZ::SerializeContext::ClassElement*> elements;
        bool allTypesValid = true;

        auto elementTypeEnumCallback =
            [&elements, &allTypesValid](const AZ::Uuid&, const AZ::SerializeContext::ClassElement* genericClassElement)
        {
            if (genericClassElement->m_flags & AZ::SerializeContext::ClassElement::Flags::FLG_POINTER)
            {
                AZ_Error("python", false, "Python marshalling does not handle naked pointers; not converting the tuple");
                allTypesValid = false;
                return false;
            }

            // Empty tuples are created with one element entry with an invalid type, so we need to check for and skip that.
            // Everything with a valid type gets added.
            if (genericClassElement->m_typeId != AZ::TypeId::CreateNull())
            {
                elements.push_back(genericClassElement);
            }
            return true;
        };
        tupleDataContainer->EnumTypes(elementTypeEnumCallback);

        if (!allTypesValid)
        {
            AZ_Error("python", false, "Could not convert tuple elements.");
            return AZStd::nullopt;
        }

        // load python items into tuple elements. If the input is a PythonProxyObject, keep a copy of the object returned
        // for each tuple value, not just its pointer, so that it doesn't get deallocated while we're converting the data.
        AZStd::vector<pybind11::object> proxyItems;
        AZStd::vector<PyObject*> items;

        if (IsValidList(pyObj))
        {
            // Python list, just grab raw pointers to each object.
            pybind11::list pyList(pyObj);
            for (size_t listIdx = 0; listIdx < pyList.size(); listIdx++)
            {
                items.push_back(pyList[listIdx].ptr());
            }
        }
        else if (IsValidTuple(pyObj))
        {
            // Python tuple, just grab raw pointers to each object.
            pybind11::tuple pyTuple(pyObj);
            for (size_t tupleIdx = 0; tupleIdx < pyTuple.size(); tupleIdx++)
            {
                items.push_back(pyTuple[tupleIdx].ptr());
            }
        }
        else if (IsCompatibleProxy(pyObj))
        {
            // Python Proxy Object that's a C++ tuple. This is a bit more complicated, because there's no easy way to detect
            // the number of properties and get them in the right order. 
            // OnDemandReflection<AZStd::tuple<T...>> exposes "GetN" in the proxy object, so we'll keep calling that with increasing
            // numbers until it stops working.
            EditorPythonBindings::PythonProxyObject* proxy = pybind11::cast<EditorPythonBindings::PythonProxyObject*>(pyObj);
            bool propertyFound = true;
            do
            {
                // Generate method names like Get0(), Get1(), Get2(), etc. and call them.
                constexpr AZStd::size_t MaxPropertyNameSize = 32;
                auto propertyName = AZStd::fixed_string<MaxPropertyNameSize>::format("Get%zu", items.size());
                auto item = proxy->Invoke(propertyName.c_str(), {});

                // For each item that's returned, save both a copy of the object to keep it from deallocating, and the raw pointer
                // that we'll use for the conversion step.
                if (!item.is_none())
                {
                    proxyItems.push_back(item);
                    items.push_back(item.ptr());
                }
                else
                {
                    propertyFound = false;
                }
            } while (propertyFound);
        }

        if (elements.size() != items.size())
        {
            AZ_Error("python", false, "Tuple requires %zu elements but received %zu elements.", elements.size(), items.size());
            return AZStd::nullopt;
        }

        // For each object found, create a copy of the value as the correct element in the C++ tuple.
        // Also, save the pointers for each allocation that we do so that we can free them in the case of an error during conversion.
        AZStd::vector<void*> reservedElements;
        for (size_t itemIdx = 0; itemIdx < elements.size(); itemIdx++)
        {
            bool successfulConversion = true;

            // Allocate space for each element.
            void* reserved = tupleDataContainer->ReserveElement(tupleInstance.m_address, elements[itemIdx]);
            if (reserved)
            {
                // Track the allocated space.
                reservedElements.push_back(reserved);
            }
            else
            {
                AZ_Error("python", reserved, "Could not allocate tuple's element %zu via ReserveElement()", itemIdx);
                successfulConversion = false;
            }

            // Attempt to convert the value. If it fails to convert, free everything we've allocated so far and return.
            if (items[itemIdx] &&
                !LoadPythonToTupleElement(
                    items[itemIdx], traits, elements[itemIdx], tupleDataContainer, itemIdx, serializeContext, tupleInstance.m_address))
            {
                successfulConversion = false;
            }

            if (!successfulConversion)
            {
                for (auto& reservedElement : reservedElements)
                {
                    tupleDataContainer->FreeReservedElement(tupleInstance.m_address, reservedElement, serializeContext);
                }
                return AZStd::nullopt;
            }
        }

        outValue.m_value = tupleInstance.m_address;
        outValue.m_typeId = tupleInstance.m_typeId;
        outValue.m_traits = traits;

        auto tupleInstanceDeleter = [behaviorClass, tupleInstance]()
        {
            behaviorClass->Destroy(tupleInstance);
        };

        return PythonMarshalTypeRequests::BehaviorValueResult{ true, tupleInstanceDeleter };
    }

    // Convert a C++ tuple into a Python list.
    AZStd::optional<PythonMarshalTypeRequests::PythonValueResult> TypeConverterTuple::BehaviorValueParameterToPython(
        AZ::BehaviorArgument& behaviorValue)
    {
        // the class data must have a container interface
        AZ::SerializeContext::IDataContainer* containerInterface = m_classData->m_container;

        if (!containerInterface)
        {
            AZ_Warning("python", false, "Container interface is missing from class %s.", m_classData->m_name);
            return AZStd::nullopt;
        }

        if (!behaviorValue.ConvertTo(m_typeId))
        {
            AZ_Warning("python", false, "Cannot convert behavior value %s.", behaviorValue.m_name);
            return AZStd::nullopt;
        }

        auto cleanUpList = AZStd::make_shared<AZStd::vector<PythonMarshalTypeRequests::DeallocateFunction>>();

        // return tuple as python tuple - if conversion fails for an element it will remain as 'none'
        size_t tupleSize = containerInterface->Size(behaviorValue.m_value);
        pybind11::tuple pythonTuple(tupleSize);
        size_t tupleElementIndex = 0;
                
        auto tupleElementCallback = [cleanUpList, tupleSize, &pythonTuple, &tupleElementIndex]
            (void* instancePair, const AZ::Uuid& elementClassId,
                [[maybe_unused]] const AZ::SerializeContext::ClassData* elementGenericClassData,
                [[maybe_unused]] const AZ::SerializeContext::ClassElement* genericClassElement)
        {
            AZ::BehaviorObject behaviorObjectValue(instancePair, elementClassId);
            auto result = Container::ProcessBehaviorObject(behaviorObjectValue);

            if (tupleElementIndex >= tupleSize)
            {
                // We've ended up with too many elements in the tuple somehow.
                AZ_Error("python", false, "Tuple contains more than the expected number of elements (%zu).", tupleSize);
                return false;
            }

            if (result.has_value())
            {
                // If the element was converted, we'll put the converted value into the output tuple.
                PythonMarshalTypeRequests::DeallocateFunction deallocateFunction = result.value().second;
                if (result.value().second)
                {
                    // If it has a deallocate function, we'll add that to our list of deallocators to get called on cleanup.
                    cleanUpList->emplace_back(AZStd::move(result.value().second));
                }

                pybind11::object pythonResult = result.value().first;
                pythonTuple[tupleElementIndex] = pythonResult;
            }
            else
            {
                // The element couldn't be converted, so we'll add 'none' as a placeholder in the tuple.
                AZ_Warning("python", false, "BehaviorObject was not processed, python item will remain 'none'.");
                pythonTuple[tupleElementIndex] = pybind11::none();
            }

            tupleElementIndex++;

            return true;
        };
        containerInterface->EnumElements(behaviorValue.m_value, tupleElementCallback);

        PythonMarshalTypeRequests::PythonValueResult result;
        result.first = pythonTuple;

        if (!cleanUpList->empty())
        {
            AZStd::weak_ptr<AZStd::vector<PythonMarshalTypeRequests::DeallocateFunction>> cleanUp(cleanUpList);
            result.second = [cleanUp]()
            {
                auto cleanupList = cleanUp.lock();
                if (cleanupList)
                {
                    AZStd::for_each(cleanupList->begin(), cleanupList->end(), [](auto& deleteMe) { deleteMe(); });
                }
            };
        }

        return result;
    }
}
