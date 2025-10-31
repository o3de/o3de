/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/map.h>
#include <AzCore/std/parallel/mutex.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/MethodConfiguration.h>
#include <ScriptCanvas/Grammar/Primitives.h>

namespace AZ
{
    class BehaviorClass;
    class BehaviorMethod;
}

namespace ScriptCanvas
{
    AZ::Outcome<void, AZStd::string> IsExposable(const AZ::BehaviorMethod& method);

    Grammar::FunctionPrototype ToSignature(const AZ::BehaviorMethod& method);

    namespace Nodes
    {
        namespace Core
        {
            class Method
                : public Node
            {
            public:
                AZ_COMPONENT(Method, "{E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF}", Node);

                static void Reflect(AZ::ReflectContext* reflectContext);

                Method() = default;

                ~Method() override = default;

                bool BranchesOnResult() const;

                size_t GenerateFingerprint() const override;

                bool CanAcceptNullInput(const Slot& executionSlot, const Slot& inputSlot) const override;

                bool GetBranchOnResultCheckName(AZStd::string& exposedName, Grammar::LexicalScope& lexicalScope) const;

                virtual bool GetCheckedOperationInfo(AZ::CheckedOperationInfo& checkedInfo, AZStd::string& exposedName, Grammar::LexicalScope& lexicalScope) const;

                const AZ::BehaviorClass* GetClass() const;

                const Slot* GetIfBranchFalseOutSlot() const override;

                const Slot* GetIfBranchTrueOutSlot() const override;

                const AZ::BehaviorMethod* GetMethod() const;

                AZ_INLINE const AZStd::string& GetMethodClassName() const { return m_classNamePretty; }

                AZ_INLINE MethodType GetMethodType() const { return m_methodType; }

                AZ_INLINE AZStd::string GetNodeName() const override { return GetName(); }

                AZ_INLINE const AZStd::string& GetName() const { return m_lookupName; }

                AZ_INLINE AZStd::string GetRawMethodName() const { return m_method ? m_method->m_name : ""; }

                AZ_INLINE const AZStd::string& GetRawMethodClassName() const { return m_className; }

                Data::Type GetResultType() const;

                bool IsCheckedOperation(bool* callCheckedOpOnBothBothBranches = nullptr) const;

                bool IsIfBranch() const override;

                bool IsIfBranchPrefacedWithBooleanExpression() const override;

                bool IsMethod(AZStd::string_view methodName) const { return m_method && m_method->m_name.compare(methodName) == 0; }

                bool IsObjectClass(AZStd::string_view objectClass) const { return objectClass.compare(m_className) == 0; }

                //! Attempts to initialize node with a BehaviorContext BehaviorMethod
                //! 1) If the names match an overloaded method, including one using ExplicitOverloadInfo, then that method is used. Else:
                //! 2) If the class name is empty, then search for a free method is searched for in the BehaviorContext and there is a warning if not found.
                //! 3) If the class name matches an ebus, methodName is searched among the BehaviorEBus events, and there is a warning if not found.
                //! 4) if the class name does NOT match an ebus, className and methodName are used to look for a registered BehaviorClass in the BehaviorContext, and there is a warning if not found.
                void InitializeBehaviorMethod(const NamespacePath& namespaces, AZStd::string_view className, AZStd::string_view methodName, PropertyStatus propertyStatus);

                void InitializeClass(const NamespacePath& namespaces, AZStd::string_view className, AZStd::string_view methodName, PropertyStatus propertyStatus);

                void InitializeEvent(const NamespacePath& namespaces, AZStd::string_view busName, AZStd::string_view eventName);

                void InitializeFree(const NamespacePath& namespaces, AZStd::string_view methodName);

                bool InitializeOverloaded(const NamespacePath& namespaces, AZStd::string_view className, AZStd::string_view methodName);

                AZ_INLINE bool IsValid() const { return m_method != nullptr; }

                bool HasBusID() const { return (m_method == nullptr) ? false : m_method->HasBusId(); }

                bool HasResult() const { return (m_method == nullptr) ? false : m_method->HasResult(); }

                SlotId GetBusSlotId() const;

                void OnDeserialize() override;

#if defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)////
                void OnWriteEnd();
#endif//defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)

                virtual bool IsMethodOverloaded() const { return false; }

                bool IsOutOfDate(const VersionData& graphVersion) const override;

                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

                AZ::Outcome<Grammar::LexicalScope, void> GetFunctionCallLexicalScope(const Slot* /*slot*/) const override;

                AZ::Outcome<AZStd::string, void> GetFunctionCallName(const Slot* slot) const override;

                EventType GetFunctionEventType(const Slot* slot) const override;

                virtual DynamicDataType GetOverloadedOutputType(size_t resultIndex) const;

                PropertyStatus GetPropertyStatus() const;

            protected:
                void ConfigureMethod(const AZ::BehaviorMethod& method, const AZ::BehaviorClass* bcClass);

                const Slot* GetIfBranchSlot(bool branch) const;

                AZ_INLINE const AZStd::string& GetLookupName() const { return m_lookupName; }

                AZ_INLINE AZStd::recursive_mutex& GetMutex() { return m_mutex; }

                ConstSlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot& executionSlot, CombinedSlotType targetSlotType, const Slot* executionChildSlot) const override;

                AZ::Outcome<Grammar::FunctionPrototype> GetSimpleSignature() const override;

                AZStd::tuple<const AZ::BehaviorMethod*, MethodType, EventType, const AZ::BehaviorClass*> LookupMethod() const;

                virtual SlotId AddMethodInputSlot(const MethodConfiguration& config, size_t argumentIndex);

                void InitializeOutput(const MethodConfiguration& config);

                void InitializeInput(const MethodConfiguration& config);

                virtual void InitializeMethod(const MethodConfiguration& config);

                AZ_INLINE bool IsConfigured() const { return m_method != nullptr; }

                bool IsExpectingResult() const;

                virtual void OnInitializeOutputPost(const MethodOutputConfig&) {}

                virtual void OnInitializeOutputPre(MethodOutputConfig&) {}

                bool SanityCheckBranchOnResultMethod(const AZ::BehaviorMethod& branchOnResultMethod) const;

                AZ_INLINE void SetClassNamePretty(AZStd::string_view classNamePretty) { m_classNamePretty = classNamePretty; }

                void SetMethodUnchecked(const AZ::BehaviorMethod* method, const AZ::BehaviorClass* behaviorClass);

                AZ_INLINE void SetWarnOnMissingFunction(bool enabled) { m_warnOnMissingFunction = enabled; }

                bool GetBehaviorContextClassMethod(const AZ::BehaviorClass*& outClass, const AZ::BehaviorMethod*& outMethod, EventType& outType) const;

                bool IsDeprecated() const override;

            private:
                friend struct ScriptCanvas::BehaviorContextMethodHelper;

                MethodType m_methodType = MethodType::Count;
                EventType m_eventType = EventType::Count;
                AZStd::string m_lookupName;
                AZStd::string m_className;
                AZStd::string m_classNamePretty;
                NamespacePath m_namespaces;
                const AZ::BehaviorMethod* m_method = nullptr;
                const AZ::BehaviorClass* m_class = nullptr;
                AZStd::vector<SlotId> m_inputSlots;
                AZStd::vector<SlotId> m_resultSlotIDs;
                AZStd::recursive_mutex m_mutex; // post-serialization
                bool m_warnOnMissingFunction = false;
                Method(const Method&) = delete;
            };

        }
    }
}
