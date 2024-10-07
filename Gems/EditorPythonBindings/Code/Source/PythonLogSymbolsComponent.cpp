/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PythonLogSymbolsComponent.h>

#include <EditorPythonBindings/PythonCommon.h>
#include <EditorPythonBindings/PythonUtility.h>
#include <Source/PythonProxyBus.h>
#include <Source/PythonProxyObject.h>
#include <Source/PythonTypeCasters.h>
#include <pybind11/embed.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/PlatformDef.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/sort.h>

#include <AzFramework/CommandLine/CommandRegistrationBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace EditorPythonBindings
{
    namespace Internal
    {
        struct FileHandle final
        {
            explicit FileHandle(AZ::IO::HandleType handle)
                : m_handle(handle)
            {
            }

            ~FileHandle()
            {
                Close();
            }

            void Close()
            {
                if (IsValid())
                {
                    AZ::IO::FileIOBase::GetInstance()->Close(m_handle);
                }
                m_handle = AZ::IO::InvalidHandle;
            }

            bool IsValid() const
            {
                return m_handle != AZ::IO::InvalidHandle;
            }

            operator AZ::IO::HandleType() const
            {
                return m_handle;
            }

            AZ::IO::HandleType m_handle;
        };
    } // namespace Internal

    void PythonLogSymbolsComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto&& serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PythonLogSymbolsComponent, AZ::Component>()->Version(0);
        }
    }

    void PythonLogSymbolsComponent::Activate()
    {
        PythonSymbolEventBus::Handler::BusConnect();
        EditorPythonBindingsNotificationBus::Handler::BusConnect();
        AZ::Interface<AzToolsFramework::EditorPythonConsoleInterface>::Register(this);

        if (PythonSymbolEventBus::GetTotalNumOfEventHandlers() > 1)
        {
            OnPostInitialize();
        }
    }

    void PythonLogSymbolsComponent::Deactivate()
    {
        AZ::Interface<AzToolsFramework::EditorPythonConsoleInterface>::Unregister(this);
        PythonSymbolEventBus::Handler::BusDisconnect();
        EditorPythonBindingsNotificationBus::Handler::BusDisconnect();
    }

    void PythonLogSymbolsComponent::OnPostInitialize()
    {
        m_basePath.clear();
        if (AZ::IO::FileIOBase::GetInstance()->GetAlias("@user@"))
        {
            // clear out the previous symbols path
            char pythonSymbolsPath[AZ_MAX_PATH_LEN];
            AZ::IO::FileIOBase::GetInstance()->ResolvePath("@user@/python_symbols", pythonSymbolsPath, AZ_MAX_PATH_LEN);
            AZ::IO::FileIOBase::GetInstance()->CreatePath(pythonSymbolsPath);
            m_basePath = pythonSymbolsPath;
        }
        EditorPythonBindingsNotificationBus::Handler::BusDisconnect();
        PythonSymbolEventBus::ExecuteQueuedEvents();
    }

    AZStd::string_view PythonLogSymbolsComponent::FetchPythonTypeAndTraits(const AZ::TypeId& typeId, AZ::u32 traits)
    {
        return m_pythonBehaviorDescription.FetchPythonTypeAndTraits(typeId, traits);
    }

    AZStd::string PythonLogSymbolsComponent::FetchPythonTypeName(const AZ::BehaviorParameter& param)
    {
        return m_pythonBehaviorDescription.FetchPythonTypeName(param);
    }

    void PythonLogSymbolsComponent::WriteMethod(
        AZ::IO::HandleType handle,
        AZStd::string_view methodName,
        const AZ::BehaviorMethod& behaviorMethod,
        const AZ::BehaviorClass* behaviorClass)
    {
        AZStd::string buffer = m_pythonBehaviorDescription.MethodDefinition(methodName, behaviorMethod, behaviorClass);
        AZ::IO::FileIOBase::GetInstance()->Write(handle, buffer.c_str(), buffer.size());
    }

    void PythonLogSymbolsComponent::WriteProperty(
        AZ::IO::HandleType handle,
        int level,
        AZStd::string_view propertyName,
        const AZ::BehaviorProperty& property,
        const AZ::BehaviorClass* behaviorClass)
    {
        AZStd::string buffer = m_pythonBehaviorDescription.PropertyDefinition(propertyName, level, property, behaviorClass);
        AZ::IO::FileIOBase::GetInstance()->Write(handle, buffer.c_str(), buffer.size());
    }

    void PythonLogSymbolsComponent::LogClass(const AZStd::string moduleName, const AZ::BehaviorClass* behaviorClass)
    {
        LogClassWithName(moduleName, behaviorClass, behaviorClass->m_name.c_str());
    }

    void PythonLogSymbolsComponent::LogClassWithName(
        const AZStd::string moduleName, const AZ::BehaviorClass* behaviorClass, const AZStd::string className)
    {
        auto fileHandle = OpenModuleAt(moduleName);
        if (fileHandle->IsValid())
        {
            AZStd::string buffer = m_pythonBehaviorDescription.ClassDefinition(behaviorClass, className);
            AZ::IO::FileIOBase::GetInstance()->Write(*fileHandle, buffer.c_str(), buffer.size());
        }
    }

    void PythonLogSymbolsComponent::LogClassMethod(
        const AZStd::string moduleName,
        const AZStd::string globalMethodName,
        [[maybe_unused]] const AZ::BehaviorClass* behaviorClass,
        const AZ::BehaviorMethod* behaviorMethod)
    {
        auto fileHandle = OpenModuleAt(moduleName);
        if (fileHandle->IsValid())
        {
            WriteMethod(*fileHandle, globalMethodName, *behaviorMethod, nullptr);
        }
    }

    void PythonLogSymbolsComponent::LogBus(
        const AZStd::string moduleName, const AZStd::string busName, const AZ::BehaviorEBus* behaviorEBus)
    {
        if (!behaviorEBus || behaviorEBus->m_events.empty())
        {
            return;
        }

        auto fileHandle = OpenModuleAt(moduleName);
        if (fileHandle->IsValid())
        {
            AZStd::string buffer = m_pythonBehaviorDescription.BusDefinition(busName, behaviorEBus);
            AZ::IO::FileIOBase::GetInstance()->Write(*fileHandle, buffer.c_str(), buffer.size());
        }
    }

    void PythonLogSymbolsComponent::LogGlobalMethod(
        const AZStd::string moduleName, const AZStd::string methodName, const AZ::BehaviorMethod* behaviorMethod)
    {
        auto fileHandle = OpenModuleAt(moduleName);
        if (fileHandle->IsValid())
        {
            WriteMethod(*fileHandle, methodName, *behaviorMethod, nullptr);
        }

        auto functionMapIt = m_globalFunctionMap.find(moduleName);
        if (functionMapIt == m_globalFunctionMap.end())
        {
            auto moduleSetIt = m_moduleSet.find(moduleName);
            if (moduleSetIt != m_moduleSet.end())
            {
                m_globalFunctionMap[*moduleSetIt] = { AZStd::make_pair(behaviorMethod, methodName) };
            }
        }
        else
        {
            GlobalFunctionList& globalFunctionList = functionMapIt->second;
            globalFunctionList.emplace_back(AZStd::make_pair(behaviorMethod, methodName));
        }
    }

    void PythonLogSymbolsComponent::LogGlobalProperty(
        const AZStd::string moduleName, const AZStd::string propertyName, const AZ::BehaviorProperty* behaviorProperty)
    {
        if (!behaviorProperty || !behaviorProperty->m_getter || !behaviorProperty->m_getter->GetResult())
        {
            return;
        }

        auto fileHandle = OpenModuleAt(moduleName);
        if (fileHandle->IsValid())
        {
            // add header
            AZ::u64 filesize = 0;
            AZ::IO::FileIOBase::GetInstance()->Size(fileHandle->m_handle, filesize);
            const bool needsHeader = (filesize == 0);

            AZStd::string buffer =
                m_pythonBehaviorDescription.GlobalPropertyDefinition(moduleName, propertyName, *behaviorProperty, needsHeader);

            AZ::IO::FileIOBase::GetInstance()->Write(*fileHandle, buffer.c_str(), buffer.size());
        }
    }

    void PythonLogSymbolsComponent::Finalize()
    {
        auto fileHandle = OpenInitFileAt("azlmbr.bus");
        if (fileHandle->IsValid())
        {
            AZStd::string buffer;
            AzFramework::StringFunc::Append(buffer, "# Bus dispatch types:\n");
            AzFramework::StringFunc::Append(buffer, "from typing_extensions import Final\n");
            AzFramework::StringFunc::Append(buffer, "Broadcast: Final[int] = 0\n");
            AzFramework::StringFunc::Append(buffer, "Event: Final[int] = 1\n");
            AzFramework::StringFunc::Append(buffer, "QueueBroadcast: Final[int] = 2\n");
            AzFramework::StringFunc::Append(buffer, "QueueEvent: Final[int] = 3\n");
            AZ::IO::FileIOBase::GetInstance()->Write(*fileHandle, buffer.c_str(), buffer.size());
        }
        fileHandle->Close();
    }

    void PythonLogSymbolsComponent::GetModuleList(AZStd::vector<AZStd::string_view>& moduleList) const
    {
        moduleList.clear();
        moduleList.reserve(m_moduleSet.size());
        AZStd::copy(m_moduleSet.begin(), m_moduleSet.end(), AZStd::back_inserter(moduleList));
    }

    void PythonLogSymbolsComponent::GetGlobalFunctionList(GlobalFunctionCollection& globalFunctionCollection) const
    {
        globalFunctionCollection.clear();

        for (const auto& globalFunctionMapEntry : m_globalFunctionMap)
        {
            const AZStd::string_view moduleName{ globalFunctionMapEntry.first };
            const GlobalFunctionList& moduleFunctionList = globalFunctionMapEntry.second;

            AZStd::transform(
                moduleFunctionList.begin(),
                moduleFunctionList.end(),
                AZStd::back_inserter(globalFunctionCollection),
                [moduleName](auto& entry) -> auto
                {
                    const GlobalFunctionEntry& globalFunctionEntry = entry;
                    const AZ::BehaviorMethod* behaviorMethod = entry.first;
                    return AzToolsFramework::EditorPythonConsoleInterface::GlobalFunction(
                        { moduleName, globalFunctionEntry.second, behaviorMethod->m_debugDescription });
                });
        }
    }

    PythonLogSymbolsComponent::FileHandlePtr PythonLogSymbolsComponent::OpenInitFileAt(AZStd::string_view moduleName)
    {
        if (m_basePath.empty())
        {
            return AZStd::make_shared<Internal::FileHandle>(AZ::IO::InvalidHandle);
        }

        // creates the __init__.py file in this path
        AZStd::string modulePath(moduleName);
        AzFramework::StringFunc::Replace(modulePath, ".", AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);

        AZStd::string initFile;
        AzFramework::StringFunc::Path::Join(m_basePath.c_str(), modulePath.c_str(), initFile);
        AzFramework::StringFunc::Append(initFile, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
        AzFramework::StringFunc::Append(initFile, "__init__.pyi");

        AZ::IO::OpenMode openMode = AZ::IO::OpenMode::ModeText | AZ::IO::OpenMode::ModeWrite;
        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        AZ::IO::Result result = AZ::IO::FileIOBase::GetInstance()->Open(initFile.c_str(), openMode, fileHandle);
        if (result)
        {
            return AZStd::make_shared<Internal::FileHandle>(fileHandle);
        }

        return AZStd::make_shared<Internal::FileHandle>(AZ::IO::InvalidHandle);
    }

    PythonLogSymbolsComponent::FileHandlePtr PythonLogSymbolsComponent::OpenModuleAt(AZStd::string_view moduleName)
    {
        if (m_basePath.empty())
        {
            return AZStd::make_shared<Internal::FileHandle>(AZ::IO::InvalidHandle);
        }

        bool resetFile = false;
        if (m_moduleSet.find(moduleName) == m_moduleSet.end())
        {
            m_moduleSet.insert(moduleName);
            resetFile = true;
        }

        AZStd::vector<AZStd::string> moduleParts;
        AzFramework::StringFunc::Tokenize(moduleName.data(), moduleParts, '.');

        // prepare target PYI file
        AZStd::string targetModule = moduleParts.back();
        moduleParts.pop_back();
        AzFramework::StringFunc::Append(targetModule, ".pyi");

        // create an __init__.py file as the base module path
        AZStd::string initModule;
        AzFramework::StringFunc::Join(initModule, moduleParts.begin(), moduleParts.end(), '.');
        OpenInitFileAt(initModule);

        AZStd::string modulePath;
        AzFramework::StringFunc::Append(modulePath, m_basePath.c_str());
        AzFramework::StringFunc::Append(modulePath, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
        AzFramework::StringFunc::Join(modulePath, moduleParts.begin(), moduleParts.end(), AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);

        // prepare the path
        AZ::IO::FileIOBase::GetInstance()->CreatePath(modulePath.c_str());

        // assemble the file path
        AzFramework::StringFunc::Append(modulePath, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
        AzFramework::StringFunc::Append(modulePath, targetModule.c_str());
        AzFramework::StringFunc::AssetDatabasePath::Normalize(modulePath);

        AZ::IO::OpenMode openMode = AZ::IO::OpenMode::ModeText;
        if (AZ::IO::SystemFile::Exists(modulePath.c_str()))
        {
            openMode |= (resetFile) ? AZ::IO::OpenMode::ModeWrite : AZ::IO::OpenMode::ModeAppend;
        }
        else
        {
            openMode |= AZ::IO::OpenMode::ModeWrite;
        }

        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        AZ::IO::Result result = AZ::IO::FileIOBase::GetInstance()->Open(modulePath.c_str(), openMode, fileHandle);
        if (result)
        {
            return AZStd::make_shared<Internal::FileHandle>(fileHandle);
        }

        return AZStd::make_shared<Internal::FileHandle>(AZ::IO::InvalidHandle);
    }
} // namespace EditorPythonBindings
