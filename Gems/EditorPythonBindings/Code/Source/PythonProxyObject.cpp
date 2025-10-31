/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PythonProxyObject.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <EditorPythonBindings/PythonUtility.h>
#include <EditorPythonBindings/PythonCommon.h>
#include <Source/PythonMarshalComponent.h>
#include <Source/PythonSymbolsBus.h>
#include <Source/PythonTypeCasters.h>

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <pybind11/eval.h>

#include <AzCore/PlatformDef.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonSerializationSettings.h>
#include <AzCore/Serialization/Json/JsonUtils.h>

namespace EditorPythonBindings
{
    namespace Operator
    {
        constexpr const char s_isEqual[] = "__eq__";
        constexpr const char s_notEqual[] = "__ne__";
        constexpr const char s_greaterThan[] = "__gt__";
        constexpr const char s_greaterThanOrEqual[] = "__ge__";
        constexpr const char s_lessThan[] = "__lt__";
        constexpr const char s_lessThanOrEqual[] = "__le__";
    }

    namespace Builtins
    {
        constexpr const char s_repr[] = "__repr__";
        constexpr const char s_str[] = "__str__";
    }

    namespace Naming
    {
        void StripReplace(AZStd::string& inout, AZStd::string_view prefix, char bracketIn, char bracketOut, AZStd::string_view replacement)
        {
            size_t pos = inout.find(prefix);
            while (pos != AZStd::string::npos)
            {
                const char* const start = &inout[pos];
                pos += prefix.size();
                const char* end = &inout[pos];
                int bracketCount = 1;
                do
                {
                    if (pos == inout.size())
                    {
                        break;
                    }
                    else if (inout[pos] == bracketIn)
                    {
                        bracketCount++;
                    }
                    else if (inout[pos] == bracketOut)
                    {
                        bracketCount--;
                    }
                    end++;
                    pos++;
                }
                while (bracketCount > 0);

                AZStd::string target{ start, end };
                AZ::StringFunc::Replace(inout, target.c_str(), replacement.data());

                pos = inout.find(prefix);
            }
        }

        AZStd::optional<AZStd::string> GetPythonSyntax(const AZ::BehaviorClass& behaviorClass)
        {
            constexpr const char* invalidCharacters = " :<>,*&";
            if (behaviorClass.m_name.find_first_of(invalidCharacters) == AZStd::string::npos)
            {
                // this class name is not using invalid characters
                return AZStd::nullopt;
            }

            AZStd::string syntaxName = behaviorClass.m_name;

            // replace common core template types and name spaces like AZStd
            StripReplace(syntaxName, "AZStd::basic_string<", '<', '>', "string");
            AZ::StringFunc::Replace(syntaxName, "AZStd", "");

            AZStd::vector<AZStd::string> tokens;
            AZ::StringFunc::Tokenize(syntaxName, tokens, invalidCharacters, false, false);
            syntaxName.clear();
            AZ::StringFunc::Join(syntaxName, tokens.begin(), tokens.end(), "_");
            return syntaxName;
        }
    }

    PythonProxyObject::PythonProxyObject(const AZ::TypeId& typeId)
    {
        const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(typeId);
        if (behaviorClass)
        {
            CreateDefault(behaviorClass);
        }
    }

    PythonProxyObject::PythonProxyObject(const char* typeName)
    {
        SetByTypeName(typeName);
    }

    pybind11::object PythonProxyObject::Construct(const AZ::BehaviorClass& behaviorClass, pybind11::args args)
    {
        // nothing to construct with ...
        if (args.size() == 0 || behaviorClass.m_constructors.empty())
        {
            if (!CreateDefault(&behaviorClass))
            {
                return pybind11::cast<pybind11::none>(Py_None);
            }
            return pybind11::cast(this);
        }

        // find the right constructor
        for (AZ::BehaviorMethod* constructor : behaviorClass.m_constructors)
        {
            const size_t numArgsPlusSelf = args.size() + 1;
            AZ_Error("python", constructor, "Missing constructor value in behavior class %s", behaviorClass.m_name.c_str());
            if (constructor && constructor->GetNumArguments() == numArgsPlusSelf)
            {
                bool match = true;
                for (size_t index = 0; index < args.size(); ++index)
                {
                    const AZ::BehaviorParameter* behaviorArg = constructor->GetArgument(index + 1);
                    pybind11::object pythonArg = args[index];
                    if (!behaviorArg || !CanConvertPythonToBehaviorValue(*behaviorArg, pythonArg))
                    {
                        match = false;
                        break;
                    }
                }

                if (match)
                {
                    // prepare wrapped object instance
                    m_wrappedObject.m_address = behaviorClass.Allocate();
                    m_wrappedObject.m_typeId = behaviorClass.m_typeId;
                    PrepareWrappedObject(behaviorClass);

                    // execute constructor
                    Call::ClassMethod(constructor, m_wrappedObject, args);
                    return pybind11::cast(this);
                }
            }
        }
        return pybind11::cast<pybind11::none>(Py_None);
    }

    bool PythonProxyObject::CanConvertPythonToBehaviorValue(const AZ::BehaviorParameter& behaviorArg, pybind11::object pythonArg) const
    {
        bool canConvert = false;
        PythonMarshalTypeRequestBus::EventResult(
            canConvert,
            behaviorArg.m_typeId,
            &PythonMarshalTypeRequestBus::Events::CanConvertPythonToBehaviorValue,
            static_cast<PythonMarshalTypeRequests::BehaviorTraits>(behaviorArg.m_traits),
            pythonArg);

        if (canConvert)
        {
            return true;
        }

        // is already a wrapped type?
        if (pybind11::isinstance<PythonProxyObject>(pythonArg))
        {
            auto&& proxyObj = pybind11::cast<PythonProxyObject*>(pythonArg);
            if (proxyObj)
            {
                return behaviorArg.m_azRtti->IsTypeOf(proxyObj->GetWrappedType().value());
            }
        }

        return false;
    }

    PythonProxyObject::PythonProxyObject(const AZ::BehaviorObject& object)
    {
        const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(object.m_typeId);
        if (behaviorClass)
        {
            m_wrappedObject = behaviorClass->Clone(object);
            PrepareWrappedObject(*behaviorClass);
        }
    }

    PythonProxyObject::~PythonProxyObject()
    {
        ReleaseWrappedObject();
    }

    const char* PythonProxyObject::GetWrappedTypeName() const
    {
        return m_wrappedObjectTypeName.c_str();
    }

    void PythonProxyObject::SetPropertyValue(const char* attributeName, pybind11::object value)
    {
        if (!m_wrappedObject.IsValid())
        {
            PyErr_SetString(PyExc_RuntimeError, "The wrapped Proxy Object has not been setup correctly; missing call set_type()?");
            AZ_Error("python", false, "The wrapped Proxy Object has not been setup correctly; missing call set_type()?");
            return;
        }

        auto behaviorPropertyIter = m_properties.find(AZ::Crc32(attributeName));
        if (behaviorPropertyIter != m_properties.end())
        {
            AZ::BehaviorProperty* property = behaviorPropertyIter->second;
            AZ_Error("python", property->m_setter, "%s is not a writable property in class %s.", attributeName, m_wrappedObjectTypeName.c_str());
            if (property->m_setter)
            {
                EditorPythonBindings::Call::ClassMethod(property->m_setter, m_wrappedObject, pybind11::args(pybind11::make_tuple(value)));
            }
        }
    }

    pybind11::object PythonProxyObject::GetPropertyValue(const char* attributeName)
    {
        if (!m_wrappedObject.IsValid())
        {
            PyErr_SetString(PyExc_RuntimeError, "The wrapped Proxy Object has not been setup correctly; missing call set_type()?");
            AZ_Error("python", false, "The wrapped Proxy Object has not been setup correctly; missing call set_type()?");
            return pybind11::cast<pybind11::none>(Py_None);
        }

        AZ::Crc32 crcAttributeName(attributeName);

        // the attribute could refer to a method
        auto methodEntry = m_methods.find(crcAttributeName);
        if (methodEntry != m_methods.end())
        {
            AZ::BehaviorMethod* method = methodEntry->second;
            return pybind11::cpp_function([this, method](pybind11::args pythonArgs)
            {
                return EditorPythonBindings::Call::ClassMethod(method, m_wrappedObject, pythonArgs);
            });
        }

        // the attribute could refer to a property
        auto behaviorPropertyIter = m_properties.find(crcAttributeName);
        if (behaviorPropertyIter != m_properties.end())
        {
            AZ::BehaviorProperty* property = behaviorPropertyIter->second;
            AZ_Error("python", property->m_getter, "%s is not a readable property in class %s.", attributeName, m_wrappedObjectTypeName.c_str());
            if (property->m_getter)
            {
                return EditorPythonBindings::Call::ClassMethod(property->m_getter, m_wrappedObject, pybind11::args());
            }
        }

        return pybind11::cast<pybind11::none>(Py_None);
    }

    bool PythonProxyObject::SetByTypeName(const char* typeName)
    {
        const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(AZStd::string(typeName));
        if (behaviorClass)
        {
            return CreateDefault(behaviorClass);
        }
        return false;
    }

    pybind11::object PythonProxyObject::Invoke(const char* methodName, pybind11::args pythonArgs)
    {
        const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(m_wrappedObject.m_typeId);
        if (behaviorClass)
        {
            if (auto behaviorMethodIter = behaviorClass->m_methods.find(methodName);
                behaviorMethodIter != behaviorClass->m_methods.end())
            {
                AZ::BehaviorMethod* method = behaviorMethodIter->second;
                AZ_Error("python", method, "%s is not a method in class %s!", methodName, m_wrappedObjectTypeName.c_str());

                if (method && PythonProxyObjectManagement::IsMemberLike(*method, m_wrappedObject.m_typeId))
                {
                    return EditorPythonBindings::Call::ClassMethod(method, m_wrappedObject, pythonArgs);
                }
            }
            else if (behaviorClass->m_unwrapper != nullptr)
            {
                // Check if the Behavior Class acts as a wrapper for a pointer type

                AZ::BehaviorObject rawObject;
                behaviorClass->m_unwrapper(m_wrappedObject.m_address, rawObject, behaviorClass->m_unwrapperUserData);
                // Check if the rawObject object contains a valid address and typeid
                if (rawObject.IsValid())
                {
                    // Check if the specified method exist on the raw object type being wrapped
                    if (behaviorClass = AZ::BehaviorContextHelper::GetClass(rawObject.m_typeId);
                        behaviorClass != nullptr)
                    {
                        if (auto rawMethodIter = behaviorClass->m_methods.find(methodName);
                            rawMethodIter != behaviorClass->m_methods.end())
                        {
                            AZ::BehaviorMethod* method = rawMethodIter->second;
                            AZ_Error("python", method, "%s is not a method in class %s!", methodName, behaviorClass->m_name.c_str());

                            if (method && PythonProxyObjectManagement::IsMemberLike(*method, rawObject.m_typeId))
                            {
                                return EditorPythonBindings::Call::ClassMethod(method, rawObject, pythonArgs);
                            }
                        }
                    }
                }
            }
        }
        return pybind11::cast<pybind11::none>(Py_None);
    }

    AZStd::optional<AZ::TypeId> PythonProxyObject::GetWrappedType() const
    {
        if (m_wrappedObject.IsValid())
        {
            return AZStd::make_optional(m_wrappedObject.m_typeId);
        }
        return AZStd::nullopt;
    }

    AZStd::optional<AZ::BehaviorObject*> PythonProxyObject::GetBehaviorObject()
    {
        if (m_wrappedObject.IsValid())
        {
            return AZStd::make_optional(&m_wrappedObject);
        }
        return AZStd::nullopt;
    }

    void PythonProxyObject::PrepareWrappedObject(const AZ::BehaviorClass& behaviorClass)
    {
        m_ownership = Ownership::Owned;
        m_wrappedObjectTypeName = behaviorClass.m_name;

        // is this Behavior Class flagged to usage for tool bindings?
        if (!Scope::IsBehaviorFlaggedForEditor(behaviorClass.m_attributes))
        {
            return;
        }

        PopulateComparisonOperators(behaviorClass);
        PopulateMethodsAndProperties(behaviorClass);

        for (auto&& baseClassId : behaviorClass.m_baseClasses)
        {
            const AZ::BehaviorClass* baseClass = AZ::BehaviorContextHelper::GetClass(baseClassId);
            if (baseClass)
            {
                PopulateMethodsAndProperties(*baseClass);
            }
        }
    }

    void PythonProxyObject::PopulateComparisonOperators(const AZ::BehaviorClass& behaviorClass)
    {
        using namespace AZ::Script;
        for (auto&& equalMethodCandidatePair : behaviorClass.m_methods)
        {
            const AZ::AttributeArray& attributes = equalMethodCandidatePair.second->m_attributes;
            AZ::Attribute* operatorAttribute = AZ::FindAttribute(Attributes::Operator, attributes);
            if (!operatorAttribute)
            {
                continue;
            }

            Attributes::OperatorType operatorType;
            AZ::AttributeReader scopeAttributeReader(nullptr, operatorAttribute);
            if (!scopeAttributeReader.Read<Attributes::OperatorType>(operatorType))
            {
                continue;
            }

            AZ::Crc32 namedKey;
            if (operatorType == Attributes::OperatorType::Equal)
            {
                namedKey = AZ::Crc32{ Operator::s_isEqual };
            }
            else if (operatorType == Attributes::OperatorType::LessThan)
            {
                namedKey = AZ::Crc32{ Operator::s_lessThan };
            }
            else if (operatorType == Attributes::OperatorType::LessEqualThan)
            {
                namedKey = AZ::Crc32{ Operator::s_lessThanOrEqual };
            }
            else
            {
                continue;
            }

            if (m_methods.find(namedKey) == m_methods.end())
            {
                m_methods[namedKey] = equalMethodCandidatePair.second;
            }
        }
    }

    void PythonProxyObject::PopulateMethodsAndProperties(const AZ::BehaviorClass& behaviorClass)
    {
        AZStd::string baseName;

        // cache all the methods for this behavior class
        for (const auto& methodEntry : behaviorClass.m_methods)
        {
            AZ::BehaviorMethod* method = methodEntry.second;
            AZ_Error("python", method, "Missing method entry:%s value in behavior class:%s", methodEntry.first.c_str(), m_wrappedObjectTypeName.c_str());
            if (method && PythonProxyObjectManagement::IsMemberLike(*method, m_wrappedObject.m_typeId))
            {
                baseName = methodEntry.first;
                Scope::FetchScriptName(method->m_attributes, baseName);
                AZ::Crc32 namedKey(baseName);
                if (m_methods.find(namedKey) == m_methods.end())
                {
                    m_methods[namedKey] = method;
                }
                else
                {
                    AZ_TracePrintf("python", "Skipping duplicate method named %s\n", baseName.c_str());
                }
            }
        }

        // cache all the properties for this behavior class
        for (const auto& behaviorProperty : behaviorClass.m_properties)
        {
            AZ::BehaviorProperty* property = behaviorProperty.second;
            AZ_Error("python", property, "Missing property %s in behavior class:%s", behaviorProperty.first.c_str(), m_wrappedObjectTypeName.c_str());
            if (property)
            {
                baseName = behaviorProperty.first;
                Scope::FetchScriptName(property->m_attributes, baseName);
                AZ::Crc32 namedKey(baseName);
                if (m_properties.find(namedKey) == m_properties.end())
                {
                    m_properties[namedKey] = property;
                }
                else
                {
                    AZ_TracePrintf("python", "Skipping duplicate property named %s\n", baseName.c_str());
                }
            }
        }
    }

    pybind11::object PythonProxyObject::GetWrappedObjectRepr()
    {
        const AZ::Crc32 reprNamedKey { Builtins::s_repr };

        // Attempt to call the object's __repr__ implementation first to get the most accurate representation.
        AZ::BehaviorMethod* reprMethod = nullptr;
        auto methodEntry = m_methods.find(reprNamedKey);
        if (methodEntry != m_methods.end())
        {
            reprMethod = methodEntry->second;

            pybind11::object result = Call::ClassMethod(reprMethod, m_wrappedObject, pybind11::args());
            if (!result.is_none())
            {
                return result;
            }
            else
            {
                AZ_Warning("python", false, "The %s method in type (%s) did not return a valid value.", Builtins::s_repr, m_wrappedObjectTypeName.c_str());
            }
        }

        // There's no __repr__ implementation in the object, so use a basic representation and cache it.
        AZ_Warning("python", false, "The type (%s) does not implement the %s method.", m_wrappedObjectTypeName.c_str(), Builtins::s_repr);
        if (m_wrappedObjectCachedRepr.empty())
        {
            pybind11::module builtinsModule = pybind11::module::import("builtins");
            auto idFunc = builtinsModule.attr("id");
            pybind11::object resId = idFunc(this);
            AZStd::string wrappedObjectId = pybind11::str(resId).operator std::string().c_str();
            m_wrappedObjectCachedRepr = AZStd::string::format("<%s via PythonProxyObject at %s>", m_wrappedObjectTypeName.c_str(), wrappedObjectId.c_str());
        }

        return pybind11::str(m_wrappedObjectCachedRepr.c_str());
    }

    pybind11::object PythonProxyObject::GetWrappedObjectStr()
    {
        // Inspect methods with attributes to find the ToString attribute
        AZ::BehaviorMethod* strMethod = nullptr;

        using namespace AZ::Script;
        for (auto&& strMethodCandidatePair : m_methods)
        {
            const AZ::AttributeArray& attributes = strMethodCandidatePair.second->m_attributes;
            AZ::Attribute* operatorAttribute = AZ::FindAttribute(Attributes::Operator, attributes);
            if (!operatorAttribute)
            {
                continue;
            }

            Attributes::OperatorType operatorType;
            AZ::AttributeReader scopeAttributeReader(nullptr, operatorAttribute);
            if (!scopeAttributeReader.Read<Attributes::OperatorType>(operatorType))
            {
                continue;
            }

            if (operatorType == Attributes::OperatorType::ToString)
            {
                if (strMethod == nullptr)
                {
                    strMethod = strMethodCandidatePair.second;
                }
                else
                {
                    AZ_Warning("python", false, "The type (%s) has more than one method with OperatorType::ToString, using the first found.", m_wrappedObjectTypeName.c_str());
                    break;
                }
            }
        }

        if (strMethod != nullptr)
        {
            pybind11::object result = Call::ClassMethod(strMethod, m_wrappedObject, pybind11::args());
            if (!result.is_none())
            {
                return result;
            }
            else
            {
                AZ_Warning("python", false, "The %s method in type (%s) did not return a valid value.", Builtins::s_str, m_wrappedObjectTypeName.c_str());
            }
        }

        // Fallback to __repr__ because there's no __str__ implementation in the object,
        // so use a basic representation and cache it.
        AZ_TracePrintf("python", "The type (%s) does not implement the %s method or did not return a valid value, trying %s.", m_wrappedObjectTypeName.c_str(), Builtins::s_str, Builtins::s_repr);
        return GetWrappedObjectRepr();
    }

    pybind11::ssize_t PythonProxyObject::GetWrappedObjectHash()
    {
        pybind11::object result = GetWrappedObjectRepr();
        return pybind11::hash(result.release());
    }

    void PythonProxyObject::ReleaseWrappedObject()
    {
        if (m_wrappedObject.IsValid() && m_ownership == Ownership::Owned)
        {
            const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(m_wrappedObject.m_typeId);
            if (behaviorClass)
            {
                behaviorClass->Destroy(m_wrappedObject);
                m_wrappedObject = {};
                m_wrappedObjectTypeName.clear();
                m_wrappedObjectCachedRepr.clear();
                m_methods.clear();
                m_properties.clear();
            }
        }
    }

    bool PythonProxyObject::CreateDefault(const AZ::BehaviorClass* behaviorClass)
    {
        AZ_Error("python", behaviorClass, "Expecting a non-null BehaviorClass");
        if (behaviorClass)
        {
            if (Scope::IsBehaviorFlaggedForEditor(behaviorClass->m_attributes))
            {
                m_wrappedObject = behaviorClass->Create();
                PrepareWrappedObject(*behaviorClass);
                return true;
            }
            AZ_Warning("python", false, "The behavior class (%s) is not flagged for Editor use.", behaviorClass->m_name.c_str());
        }
        return false;
    }

    bool PythonProxyObject::DoEqualityEvaluation(pybind11::object pythonOther)
    {
        constexpr AZ::Crc32 namedEqKey(Operator::s_isEqual);
        auto&& equalOperatorMethodEntry = m_methods.find(namedEqKey);
        if (equalOperatorMethodEntry != m_methods.end())
        {
            AZ::BehaviorMethod* method = equalOperatorMethodEntry->second;
            pybind11::object result = Call::ClassMethod(method, m_wrappedObject, pybind11::args(pybind11::make_tuple(pythonOther)));
            if (result.is_none())
            {
                return false;
            }
            return result.cast<bool>();
        }
        return false;
    }

    pybind11::object PythonProxyObject::ToJson()
    {
        rapidjson::Document document;
        AZ::JsonSerializerSettings settings;
        settings.m_keepDefaults = true;

        auto resultCode =
            AZ::JsonSerialization::Store(document, document.GetAllocator(), m_wrappedObject.m_address, nullptr, m_wrappedObject.m_typeId, settings);

        if (resultCode.GetProcessing() == AZ::JsonSerializationResult::Processing::Halted)
        {
            AZ_Error("PythonProxyObject", false, "Failed to serialize to json");
            return pybind11::cast<pybind11::none>(Py_None);
        }

        AZStd::string jsonString;
        AZ::Outcome<void, AZStd::string> outcome = AZ::JsonSerializationUtils::WriteJsonString(document, jsonString);

        if (!outcome.IsSuccess())
        {
            AZ_Error("PythonProxyObject", false, "Failed to write json string: %s", outcome.GetError().c_str());
            return pybind11::cast<pybind11::none>(Py_None);
        }

        jsonString.erase(AZStd::remove(jsonString.begin(), jsonString.end(), '\n'), jsonString.end());
        auto pythonCode = AZStd::string::format(
            R"PYTHON(exec("import json") or json.loads("""%s"""))PYTHON", jsonString.c_str());

        return pybind11::eval(pythonCode.c_str());
    }

    bool PythonProxyObject::DoComparisonEvaluation(pybind11::object pythonOther, Comparison comparison)
    {
        bool invertLogic = false;
        AZ::Crc32 namedKey;
        if (comparison == Comparison::LessThan)
        {
            namedKey = AZ::Crc32{ Operator::s_lessThan };
        }
        else if (comparison == Comparison::LessThanOrEquals)
        {
            namedKey = AZ::Crc32{ Operator::s_lessThanOrEqual };
        }
        else if (comparison == Comparison::GreaterThan)
        {
            namedKey = AZ::Crc32{ Operator::s_lessThan };
            invertLogic = true;
        }
        else if (comparison == Comparison::GreaterThanOrEquals)
        {
            namedKey = AZ::Crc32{ Operator::s_lessThan };
            invertLogic = true;
        }
        else
        {
            return false;
        }

        auto&& equalOperatorMethodEntry = m_methods.find(namedKey);
        if (equalOperatorMethodEntry != m_methods.end())
        {
            AZ::BehaviorMethod* method = equalOperatorMethodEntry->second;
            pybind11::object result = Call::ClassMethod(method, m_wrappedObject, pybind11::args(pybind11::make_tuple(pythonOther)));
            if (result.is_none())
            {
                return false;
            }
            else if (invertLogic)
            {
                const bool greaterThanResult = !result.cast<bool>();

                // an additional check for "GreaterThanOrEquals" if the result of "LessThan" failed since the invert
                // of '3 <= 3' would fail since the 'or equals' would return true and be inverted to false
                if (comparison == Comparison::GreaterThanOrEquals && greaterThanResult == false)
                {
                    return DoEqualityEvaluation(pythonOther);
                }

                return greaterThanResult;
            }
            return result.cast<bool>();
        }
        return false;
    }

    namespace PythonProxyObjectManagement
    {
        bool IsMemberLike(const AZ::BehaviorMethod& method, const AZ::TypeId& typeId)
        {
            return method.IsMember() || (method.GetNumArguments() > 0 && method.GetArgument(0)->m_typeId == typeId);
        }

        bool IsClassConstant(const AZ::BehaviorProperty* property)
        {
            bool value = false;
            AZ::Attribute* classConstantAttribute = AZ::FindAttribute(AZ::Script::Attributes::ClassConstantValue, property->m_attributes);
            if (classConstantAttribute)
            {
                AZ::AttributeReader attributeReader(nullptr, classConstantAttribute);
                attributeReader.Read<bool>(value);
            }
            return value;
        }

        pybind11::object CreatePythonProxyObject(const AZ::TypeId& typeId, void* data)
        {
            PythonProxyObject* instance = nullptr;
            if (!data)
            {
                instance = aznew PythonProxyObject(typeId);
            }
            else
            {
                instance = aznew PythonProxyObject(AZ::BehaviorObject(data, typeId));
            }

            if (!instance->GetWrappedType())
            {
                delete instance;
                PyErr_SetString(PyExc_TypeError, "Failed to create proxy object by type name.");
                return pybind11::cast<pybind11::none>(Py_None);
            }
            return pybind11::cast(instance);
        }

        pybind11::object CreatePythonProxyObjectByTypename(const char* classTypename)
        {
            const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(AZStd::string(classTypename));
            AZ_Warning("python", behaviorClass, "Missing Behavior Class for typename:%s", classTypename);
            if (!behaviorClass)
            {
                return pybind11::cast<pybind11::none>(Py_None);
            }
            return CreatePythonProxyObject(behaviorClass->m_typeId, nullptr);
        }

        pybind11::object ConstructPythonProxyObjectByTypename(const char* classTypename, pybind11::args args)
        {
            const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(AZStd::string(classTypename));
            AZ_Warning("python", behaviorClass, "Missing Behavior Class for typename:%s", classTypename);
            if (!behaviorClass)
            {
                return pybind11::cast<pybind11::none>(Py_None);
            }

            PythonProxyObject* instance = aznew PythonProxyObject();
            pybind11::object pythonInstance = instance->Construct(*behaviorClass, args);
            if (pythonInstance.is_none())
            {
                delete instance;
                PyErr_SetString(PyExc_TypeError, "Failed to construct proxy object with provided args.");
                return pybind11::cast<pybind11::none>(Py_None);
            }
            return pybind11::cast(instance);
        }

        void ExportStaticBehaviorClassElements(pybind11::module parentModule, pybind11::module defaultModule)
        {
            AZ::BehaviorContext* behaviorContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
            AZ_Error("python", behaviorContext, "Behavior context not available");
            if (!behaviorContext)
            {
                return;
            }

            // this will make the base package modules for namespace "azlmbr.*" and "azlmbr.default" for behavior that does not specify a module name
            Module::PackageMapType modulePackageMap;

            for (const auto& classEntry : behaviorContext->m_classes)
            {
                AZ::BehaviorClass* behaviorClass = classEntry.second;

                // is this Behavior Class flagged to usage for Editor.exe bindings?
                if (!Scope::IsBehaviorFlaggedForEditor(behaviorClass->m_attributes))
                {
                    continue; // skip this class
                }

                // find the target module of the behavior's static methods
                auto moduleName = Module::GetName(behaviorClass->m_attributes);
                pybind11::module subModule = Module::DeterminePackageModule(modulePackageMap, moduleName ? *moduleName : "", parentModule, defaultModule, false);

                // early detection of instance based elements like constructors or properties
                bool hasMemberMethods = behaviorClass->m_constructors.empty() == false;
                bool hasMemberProperties = behaviorClass->m_properties.empty() == false;

                // does this class define methods that may be reflected in a Python module?
                if (!behaviorClass->m_methods.empty())
                {
                    // add the non-member methods as Python 'free' function
                    for (const auto& methodEntry : behaviorClass->m_methods)
                    {
                        const AZStd::string& methodName = methodEntry.first;
                        AZ::BehaviorMethod* behaviorMethod = methodEntry.second;
                        if (!PythonProxyObjectManagement::IsMemberLike(*behaviorMethod, behaviorClass->m_typeId))
                        {
                            // the name of the static method will be "azlmbr.<sub_module>.<Behavior Class>_<Behavior Method>"
                            AZStd::string globalMethodName = AZStd::string::format("%s_%s", behaviorClass->m_name.c_str(), methodName.c_str());

                            if (behaviorMethod->HasResult())
                            {
                                subModule.def(globalMethodName.c_str(), [behaviorMethod](pybind11::args args)
                                {
                                    return Call::StaticMethod(behaviorMethod, args);
                                });
                            }
                            else
                            {
                                subModule.def(globalMethodName.c_str(), [behaviorMethod](pybind11::args args)
                                {
                                    Call::StaticMethod(behaviorMethod, args);
                                });
                            }

                            AZStd::string subModuleName = pybind11::cast<AZStd::string>(subModule.attr("__name__"));
                            PythonSymbolEventBus::QueueBroadcast(&PythonSymbolEventBus::Events::LogClassMethod, subModuleName, globalMethodName, behaviorClass, behaviorMethod);
                        }
                        else
                        {
                            // any member method means the class should be exported to Python
                            hasMemberMethods = true;
                        }
                    }
                }

                // expose all the constant class properties for Python to use
                for (const auto& propertyEntry : behaviorClass->m_properties)
                {
                    const AZStd::string& propertyEntryName = propertyEntry.first;
                    AZ::BehaviorProperty* behaviorProperty = propertyEntry.second;

                    if (IsClassConstant(behaviorProperty))
                    {
                        // the name of the property will be "azlmbr.<Module>.<Behavior Class>_<Behavior Property>"
                        AZStd::string constantPropertyName =
                            AZStd::string::format("%s_%s", behaviorClass->m_name.c_str(), propertyEntryName.c_str());

                        pybind11::object constantValue = Call::StaticMethod(behaviorProperty->m_getter, {});
                        pybind11::setattr(subModule, constantPropertyName.c_str(), constantValue);

                        AZStd::string subModuleName = pybind11::cast<AZStd::string>(subModule.attr("__name__"));
                        PythonSymbolEventBus::QueueBroadcast(&PythonSymbolEventBus::Events::LogGlobalProperty, subModuleName, constantPropertyName, behaviorProperty);
                    }
                }

                // if the Behavior Class has any properties, methods, or constructors then export it
                const bool exportBehaviorClass = (hasMemberMethods || hasMemberProperties);

                // register all Behavior Class types with a Python function to construct an instance
                if (exportBehaviorClass)
                {
                    const char* behaviorClassName = behaviorClass->m_name.c_str();
                    subModule.attr(behaviorClassName) = pybind11::cpp_function([behaviorClassName](pybind11::args pythonArgs)
                    {
                        return ConstructPythonProxyObjectByTypename(behaviorClassName, pythonArgs);
                    });

                    AZStd::string subModuleName = pybind11::cast<AZStd::string>(subModule.attr("__name__"));

                    // register an alternative class name that passes the Python syntax
                    auto syntaxName = Naming::GetPythonSyntax(*behaviorClass);
                    if (syntaxName)
                    {
                        const char* properSyntax = syntaxName.value().c_str();
                        subModule.attr(properSyntax) = pybind11::cpp_function([behaviorClassName](pybind11::args pythonArgs)
                        {
                            return ConstructPythonProxyObjectByTypename(behaviorClassName, pythonArgs);
                        });
                        PythonSymbolEventBus::QueueBroadcast(&PythonSymbolEventBus::Events::LogClassWithName, subModuleName, behaviorClass, syntaxName.value());
                    }
                    else
                    {
                        PythonSymbolEventBus::QueueBroadcast(&PythonSymbolEventBus::Events::LogClass, subModuleName, behaviorClass);
                    }
                }
            }
        }

        pybind11::list ListBehaviorAttributes(const PythonProxyObject& pythonProxyObject)
        {
            pybind11::list items;
            AZStd::string baseName;

            auto typeId = pythonProxyObject.GetWrappedType();
            if (!typeId)
            {
                return items;
            }

            const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(typeId.value());
            if (!behaviorClass)
            {
                return items;
            }

            if (!Scope::IsBehaviorFlaggedForEditor(behaviorClass->m_attributes))
            {
                return items;
            }

            for (const auto& methodEntry : behaviorClass->m_methods)
            {
                AZ::BehaviorMethod* method = methodEntry.second;
                if (method && PythonProxyObjectManagement::IsMemberLike(*method, typeId.value()))
                {
                    baseName = methodEntry.first;
                    Scope::FetchScriptName(method->m_attributes, baseName);
                    items.append(pybind11::str(baseName.c_str()));
                }
            }

            for (const auto& behaviorProperty : behaviorClass->m_properties)
            {
                AZ::BehaviorProperty* property = behaviorProperty.second;
                if (property)
                {
                    baseName = behaviorProperty.first;
                    Scope::FetchScriptName(property->m_attributes, baseName);
                    items.append(pybind11::str(baseName.c_str()));
                }
            }
            return items;
        }

        pybind11::list ListBehaviorClasses(bool onlyIncludeScopedForAutomation)
        {
            pybind11::list items;
            AZ::BehaviorContext* behaviorContext(nullptr);
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
            if (!behaviorContext)
            {
                AZ_Error("python", false, "A behavior context is required!");
                return items;
            }

            for (auto&& classEntry : behaviorContext->m_classes)
            {
                auto&& behaviorClass = classEntry.second;
                if (onlyIncludeScopedForAutomation )
                {
                    if (Scope::IsBehaviorFlaggedForEditor(behaviorClass->m_attributes))
                    {
                        items.append(pybind11::str(classEntry.first.c_str()));
                    }
                }
                else
                {
                    items.append(pybind11::str(classEntry.first.c_str()));
                }
            }
            return items;
        }

        void CreateSubmodule(pybind11::module parentModule, pybind11::module defaultModule)
        {
            ExportStaticBehaviorClassElements(parentModule, defaultModule);

            auto objectModule = parentModule.def_submodule("object");
            objectModule.def("create", &CreatePythonProxyObjectByTypename);
            objectModule.def("construct", &ConstructPythonProxyObjectByTypename);
            objectModule.def("dir", &ListBehaviorAttributes);
            objectModule.def("list_classes", &ListBehaviorClasses, pybind11::arg("onlyIncludeScopedForAutomation") = true);

            pybind11::class_<PythonProxyObject>(objectModule, "PythonProxyObject", pybind11::dynamic_attr())
                .def(pybind11::init<>())
                .def(pybind11::init<const char*>())
                .def_property_readonly("typename", &PythonProxyObject::GetWrappedTypeName)
                .def("set_type", &PythonProxyObject::SetByTypeName)
                .def("set_property", &PythonProxyObject::SetPropertyValue)
                .def("get_property", &PythonProxyObject::GetPropertyValue)
                .def("invoke", &PythonProxyObject::Invoke)
                .def("to_json", &PythonProxyObject::ToJson)
                .def(Operator::s_isEqual, [](PythonProxyObject& self, pybind11::object rhs)
                {
                    return self.DoEqualityEvaluation(rhs);
                })
                .def(Operator::s_notEqual, [](PythonProxyObject& self, pybind11::object rhs)
                {
                    return self.DoEqualityEvaluation(rhs) == false;
                })
                .def(Operator::s_greaterThan, [](PythonProxyObject& self, pybind11::object rhs)
                {
                    return self.DoComparisonEvaluation(rhs, PythonProxyObject::Comparison::GreaterThan);
                })
                .def(Operator::s_greaterThanOrEqual, [](PythonProxyObject& self, pybind11::object rhs)
                {
                    return self.DoComparisonEvaluation(rhs, PythonProxyObject::Comparison::GreaterThanOrEquals);
                })
                .def(Operator::s_lessThan, [](PythonProxyObject& self, pybind11::object rhs)
                {
                    return self.DoComparisonEvaluation(rhs, PythonProxyObject::Comparison::LessThan);
                })
                .def(Operator::s_lessThanOrEqual, [](PythonProxyObject& self, pybind11::object rhs)
                {
                    return self.DoComparisonEvaluation(rhs, PythonProxyObject::Comparison::LessThanOrEquals);
                })
                .def("__setattr__", &PythonProxyObject::SetPropertyValue)
                .def("__getattr__", &PythonProxyObject::GetPropertyValue)
                .def("__hash__", &PythonProxyObject::GetWrappedObjectHash)
                .def(Builtins::s_repr, [](PythonProxyObject& self)
                {
                    return self.GetWrappedObjectRepr();
                })
                .def(Builtins::s_str, [](PythonProxyObject& self)
                {
                    return self.GetWrappedObjectStr();
                })
                ;
        }
    }
}
