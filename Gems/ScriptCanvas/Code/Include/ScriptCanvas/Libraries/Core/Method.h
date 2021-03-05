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

#pragma once

#include <AzCore/std/containers/map.h>
#include <AzCore/std/parallel/mutex.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>

namespace AZ
{
    class BehaviorClass;
    class BehaviorMethod;
}

namespace ScriptCanvas
{
    using Namespaces = AZStd::vector<AZStd::string>;

    AZ::Outcome<void, AZStd::string> IsExposable(const AZ::BehaviorMethod& method);

    namespace Nodes
    {
        namespace Core
        {
            enum class MethodType
            {
                Event,
                Free,
                Member,
                Count,
            };

            class Method : public Node
            {
            public:
                AZ_COMPONENT(Method, "{E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF}", Node);            

                static void Reflect(AZ::ReflectContext* reflectContext);

                static AZStd::string GetArgumentName(size_t argIndex, const AZ::BehaviorMethod& method, const AZ::BehaviorClass* bcClass, AZStd::string_view replaceTypeName="");

                Method() = default;

                ~Method() = default;

                AZ_INLINE AZStd::string GetNodeName() const override { return GetName(); }

                AZ_INLINE const AZStd::string& GetName() const { return m_lookupName; }
                AZ_INLINE AZStd::string GetRawMethodName() const { return m_method ? m_method->m_name : ""; }
                AZ_INLINE const AZStd::string& GetRawMethodClassName() const { return m_className; }

                AZ_INLINE const AZStd::string& GetMethodClassName() const { return m_classNamePretty; }
                AZ_INLINE MethodType GetMethodType() const { return m_methodType; }

                bool IsObjectClass(AZStd::string_view objectClass) const { return objectClass.compare(m_className) == 0; }
                bool IsMethod(AZStd::string_view methodName) const { return m_method && m_method->m_name.compare(methodName) == 0; }

                void InitializeClass(const Namespaces& namespaces, const AZStd::string& className, const AZStd::string& methodName);

                void InitializeClassOrBus(const Namespaces& namespaces, const AZStd::string& className, const AZStd::string& methodName);

                void InitializeEvent(const Namespaces& namespaces, const AZStd::string& busName, const AZStd::string& eventName);

                void InitializeFree(const Namespaces& namespaces, const AZStd::string& methodName);

                AZ_INLINE bool IsValid() const { return m_method != nullptr; }

                bool HasBusID() const { return (m_method == nullptr) ? false : m_method->HasBusId(); }
                bool HasResult() const { return (m_method == nullptr) ? false : m_method->HasResult(); }

                SlotId GetBusSlotId() const;

                void OnWriteEnd();
                                

            protected:

                const AZ::BehaviorMethod* GetMethod() const;
                void SetMethodUnchecked(const AZ::BehaviorMethod* method);
                                
                void ConfigureMethod(const AZ::BehaviorMethod& method);

                bool FindClass(const AZ::BehaviorMethod*& outMethod, const AZ::BehaviorClass*& outClass, const Namespaces& namespaces, AZStd::string_view className, AZStd::string_view methodName);
                bool FindEvent(const AZ::BehaviorMethod*& outMethod, const Namespaces& namespaces, AZStd::string_view busName, AZStd::string_view eventName);
                bool FindFree(const AZ::BehaviorMethod*& outMethod, const Namespaces& namespaces, AZStd::string_view methodName);
                                
                virtual SlotId AddMethodInputSlot(AZStd::string_view argName, AZStd::string_view toolTip, const AZ::BehaviorParameter& argument);
                
                struct MethodConfiguration
                {
                    const AZ::BehaviorMethod& m_method;
                    const AZ::BehaviorClass* m_class = nullptr;
                    const Namespaces* m_namespaces = nullptr;
                    const AZStd::string* m_className = nullptr;
                    const AZStd::string* m_lookupName = nullptr; // the look-up name in the class, rather than method.m_name
                    
                    MethodType m_methodType = MethodType::Count;
                    
                    AZ_INLINE MethodConfiguration(const AZ::BehaviorMethod& method, MethodType type) : m_method(method), m_methodType(type) {}
                };
                virtual void InitializeOutput(const MethodConfiguration& config);
                virtual void InitializeInput(const MethodConfiguration& config);
                virtual void InitializeMethod(const MethodConfiguration& config);
                
                AZ_INLINE bool IsConfigured() const { return m_method != nullptr; }

                bool IsExpectingResult() const;

                void OnInputSignal(const SlotId&) override;
                
                AZ_INLINE AZStd::vector<SlotId>& ModResultSlotIds() { return m_resultSlotIDs; }
                AZ_INLINE bool IsOutcomeOutputMethod() const { return m_isOutcomeOutputMethod; }
                AZ_INLINE AZStd::map<size_t, const AZ::BehaviorMethod*>& ModTupleGetMethods() { return m_tupleGetMethods; }

            private:
                friend struct ScriptCanvas::BehaviorContextMethodHelper;

                MethodType m_methodType = MethodType::Count;
                bool m_isOutcomeOutputMethod = false;
                AZStd::map<size_t, const AZ::BehaviorMethod*> m_tupleGetMethods;
                AZStd::string m_lookupName;
                AZStd::string m_className;
                AZStd::string m_classNamePretty;
                Namespaces m_namespaces;
                const AZ::BehaviorMethod* m_method = nullptr;
                const AZ::BehaviorClass* m_class = nullptr;
                AZStd::vector<SlotId> m_resultSlotIDs;
                AZStd::recursive_mutex m_mutex; // post-serialization

                Method(const Method&) = delete;
            };

        } // namespace Core
    } // namespace Nodes
} // namespace ScriptCanvas