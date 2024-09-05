/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIO.h>

#include <EditorPythonBindings/PythonCommon.h>
#include <EditorPythonBindings/PythonUtility.h>
#include <Source/PythonSymbolsBus.h>

#include <EditorPythonBindings/EditorPythonBindingsBus.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>

namespace AZ
{
    class BehaviorClass;
    class BehaviorMethod;
    class BehaviorProperty;
}

namespace EditorPythonBindings
{
    namespace Internal
    {
        struct FileHandle;
    }

    //! Exports Python symbols to the log folder for Python script developers to include into their local projects
    class PythonLogSymbolsComponent
        : public AZ::Component
        , private EditorPythonBindingsNotificationBus::Handler
        , private PythonSymbolEventBus::Handler
        , private AzToolsFramework::EditorPythonConsoleInterface
    {
    public:
        AZ_COMPONENT(PythonLogSymbolsComponent, "{F1873D04-C472-41A2-8AA4-48B0CE4A5979}", AZ::Component);

        static void Reflect(AZ::ReflectContext* context);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        ////////////////////////////////////////////////////////////////////////
        // EditorPythonBindingsNotificationBus::Handler
        void OnPostInitialize() override;

        ////////////////////////////////////////////////////////////////////////
        // PythonSymbolEventBus::Handler
        void LogClass(const AZStd::string moduleName, const AZ::BehaviorClass* behaviorClass) override;
        void LogClassWithName(const AZStd::string moduleName, const AZ::BehaviorClass* behaviorClass, const AZStd::string className) override;
        void LogClassMethod(
            const AZStd::string moduleName,
            const AZStd::string globalMethodName,
            const AZ::BehaviorClass* behaviorClass,
            const AZ::BehaviorMethod* behaviorMethod) override;
        void LogBus(const AZStd::string moduleName, const AZStd::string busName, const AZ::BehaviorEBus* behaviorEBus) override;
        void LogGlobalMethod(const AZStd::string moduleName, const AZStd::string methodName, const AZ::BehaviorMethod* behaviorMethod) override;
        void LogGlobalProperty(
            const AZStd::string moduleName,
            const AZStd::string propertyName,
            const AZ::BehaviorProperty* behaviorProperty) override;
        void Finalize() override;
        AZStd::string FetchPythonTypeName(const AZ::BehaviorParameter& param) override;

        ////////////////////////////////////////////////////////////////////////
        // EditorPythonConsoleInterface
        void GetModuleList(AZStd::vector<AZStd::string_view>& moduleList) const override;
        void GetGlobalFunctionList(GlobalFunctionCollection& globalFunctionCollection) const override;

        ////////////////////////////////////////////////////////////////////////
        // Python type deduction
        AZStd::string_view FetchPythonTypeAndTraits(const AZ::TypeId& typeId, AZ::u32 traits);

    private:

        using ModuleSet = AZStd::unordered_set<AZStd::string>;
        using GlobalFunctionEntry = AZStd::pair<const AZ::BehaviorMethod*, AZStd::string>;
        using GlobalFunctionList = AZStd::vector<GlobalFunctionEntry>;
        using GlobalFunctionMap = AZStd::unordered_map<AZStd::string_view, GlobalFunctionList>;
        using FileHandlePtr = AZStd::shared_ptr<Internal::FileHandle>;

        AZStd::string m_basePath;
        ModuleSet m_moduleSet;
        GlobalFunctionMap m_globalFunctionMap;
        Text::PythonBehaviorDescription m_pythonBehaviorDescription;

        FileHandlePtr OpenInitFileAt(AZStd::string_view moduleName);
        FileHandlePtr OpenModuleAt(AZStd::string_view moduleName);
        void WriteMethod(AZ::IO::HandleType handle, AZStd::string_view methodName, const AZ::BehaviorMethod& behaviorMethod, const AZ::BehaviorClass* behaviorClass);
        void WriteProperty(AZ::IO::HandleType handle, int level, AZStd::string_view propertyName, const AZ::BehaviorProperty& property, const AZ::BehaviorClass* behaviorClass);
    };
}
