/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: ScriptAdapterComponent.
 * Create: 2021-06-28
 */
#include <AzCore/Script/ScriptAdapterComponent.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Script/lua/lua.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/BehaviorContextUtilities.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/IO/SystemFile.h>
#if defined(WIN32) || defined(WIN64)
#include <atlsecurity.h>
#endif

namespace AZ
{
    const char* ScriptAdapterComponent::RELATIVE_LUA_PATH = "gen\\LuaAPI\\";
    const char* ScriptAdapterComponent::GEN_DIR_NAME = "gen";
    const char* ScriptAdapterComponent::GLOBAL_LUA_FILE_NAME = "GlobalsStaticLuaData.lua";
    const char* ScriptAdapterComponent::CLASS_LUA_FILE_NAME = "ClassesStaticLuaData.lua";
    const char* ScriptAdapterComponent::EBUS_LUA_FILE_NAME = "EbusesStaticLuaData.lua";
    const int ScriptAdapterComponent::NAME_BUFFER_SIZE = 64;

    void ScriptAdapterComponent::Reflect(ReflectContext* context)
    {
        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ScriptAdapterComponent, Component>()->Version(1);
        }
    }

    void ScriptAdapterComponent::Activate()
    {
        AdapterBus::Handler::BusConnect();
    }

    void ScriptAdapterComponent::Deactivate()
    {
        AdapterBus::Handler::BusDisconnect();
    }

    AZStd::string ScriptAdapterComponent::GetProjectPath()
    {
        ComponentApplication* app = nullptr;
        EBUS_EVENT_RESULT(app, ComponentApplicationBus, GetApplication);
        const char* engineRoot = app->GetEngineRoot();
        AZ_Assert(engineRoot != nullptr, "app GetEngineRoot failed");
        const auto projectName = Utils::GetProjectName();
        return AZStd::string::format("%s\\%s\\", engineRoot, projectName.c_str());
    }

    AZStd::string ScriptAdapterComponent::GetAbsoluteLuaDirPath()
    {
        return AZStd::string::format("%s%s", GetProjectPath().c_str(), RELATIVE_LUA_PATH);
    }

    AZStd::string ScriptAdapterComponent::GetGenDirPath()
    {
        return AZStd::string::format("%s%s", GetProjectPath().c_str(), GEN_DIR_NAME);
    }

    bool ScriptAdapterComponent::SetGenDirWritable(bool isWritable)
    {
    #if defined(WIN32) || defined(WIN64)
        char userNameBuffer[NAME_BUFFER_SIZE];
        DWORD userNameSize = NAME_BUFFER_SIZE;
        if(!GetUserName(userNameBuffer, &userNameSize)) {
            return false;
        }
        CDacl dacl;
        CSid sid;
        // bind user identifier
        if (!sid.LoadAccount(userNameBuffer)) {
            return false;
        }
        AZStd::string genPath = GetGenDirPath();
        if (!AtlGetDacl(genPath.c_str(), SE_FILE_OBJECT, &dacl)) {
            return false;
        }

        dacl.RemoveAllAces();
        dacl.AddAllowedAce(sid, GENERIC_READ | GENERIC_EXECUTE, OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);
        if (!isWritable)
        {
            dacl.AddDeniedAce(sid, DELETE | FILE_DELETE_CHILD | FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES, OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);
        }

        return AtlSetDacl(genPath.c_str(), SE_FILE_OBJECT, dacl) ? true : false;
    #elif defined(LINUX)
        SetGenDirWritableRecursive(GetGenDirPath().c_str(), isWritable);
        return true;
    #endif
    }

    AZStd::string ScriptAdapterComponent::GetLuaFilePathByType(LuaDataTypeId id)
    {
        AZStd::string dirPath = GetAbsoluteLuaDirPath();
        AZStd::string filePath;
        if (id == LuaDataType::GLOBAL)
        {
            filePath = AZStd::string::format("%s%s", dirPath.c_str(), GLOBAL_LUA_FILE_NAME);
        }
        else if (id == LuaDataType::CLASS)
        {
            filePath = AZStd::string::format("%s%s", dirPath.c_str(), CLASS_LUA_FILE_NAME);
        }
        else if (id == LuaDataType::EBUS)
        {
            filePath = AZStd::string::format("%s%s", dirPath.c_str(), EBUS_LUA_FILE_NAME);
        }
        else
        {
            filePath = "";
            AZ_Assert(false, "The LuaDataTypeId does not exist.");
        }
        return filePath;
    }

    void ScriptAdapterComponent::FormatEbusName(AZStd::string& name)
    {
        StringFunc::Replace(name, "-", "", true);
        StringFunc::Replace(name, " ", "", true);
    }

    AZStd::string ScriptAdapterComponent::GetFunctionString(const AZStd::string& funcName, LuaDataTypeId id)
    {
        if (id == LuaDataType::GLOBAL)
        {
            return funcName + " = function()\nend\n";
        }
        else if (id == LuaDataType::CLASS)
        {
            return "    " + funcName + " = function()\n    end,\n";
        }
        else if (id == LuaDataType::EBUS)
        {
            return "        " + funcName + " = function()\n        end,\n";
        }
        return "";
    }

    AZStd::string ScriptAdapterComponent::GetPropertyString(const AZStd::string& proName, LuaDataTypeId id)
    {
        if (id == LuaDataType::GLOBAL)
        {
            return proName + " = true\n";
        }
        else if (id == LuaDataType::CLASS)
        {
            return "    " + proName + " = true,\n";
        }
        else if (id == LuaDataType::EBUS)
        {
            return "        " + proName + " = true,\n";
        }
        return "";
    }

    void ScriptAdapterComponent::WriteStringToFileByType(const AZStd::string& fileStr, LuaDataTypeId id)
    {
        AZStd::string fPath = GetLuaFilePathByType(id);
        Utils::WriteFile(fileStr, fPath);
    }

    void ScriptAdapterComponent::SaveLuaGlobalsToFile(ScriptContext* context)
    {
        AZStd::string globalsInfoStr = "\n-- globals \n";
        lua_State* l = context->NativeContext();
        lua_rawgeti(l, LUA_REGISTRYINDEX, AZ_LUA_GLOBALS_TABLE_REF); // load the global table
        lua_pushnil(l);
        while (lua_next(l, -2) != 0)
        {
            if (lua_isstring(l, -2) && lua_isfunction(l, -1))
            {
                const char* name = lua_tostring(l, -2);
                if (lua_tocfunction(l, -1) == &Internal::LuaPropertyTagHelper) // if it's a property
                {
                    globalsInfoStr += GetPropertyString(name, LuaDataType::GLOBAL);
                }
                else
                {
                    if (strncmp(name, "__", 2) != 0)
                    {
                        globalsInfoStr += GetFunctionString(name, LuaDataType::GLOBAL);
                    }
                }
            }
            lua_pop(l, 1);
        }
        lua_pop(l, 1); // pop globals table

        WriteStringToFileByType(globalsInfoStr.c_str(), LuaDataType::GLOBAL);
    }

    void ScriptAdapterComponent::SaveLuaClassesToFile(ScriptContext* context)
    {
        AZStd::string classesInfoStr = "\n-- classes\n";
        lua_State* l = context->NativeContext();
        lua_rawgeti(l, LUA_REGISTRYINDEX, AZ_LUA_CLASS_TABLE_REF); // load the class table
        lua_pushnil(l);
        while (lua_next(l, -2) != 0)
        {
            // if the key is not userdata(class type), skip
            if (!lua_isuserdata(l, -2))
            {
                lua_pop(l, 1);
                continue;
            }

            if (lua_istable(l, -1))
            {
                lua_rawgeti(l, -1, AZ_LUA_CLASS_METATABLE_BEHAVIOR_CLASS);
                BehaviorClass* behaviorClass = reinterpret_cast<BehaviorClass*>(lua_touserdata(l, -1));

                if (Attribute* attribute = behaviorClass->FindAttribute(Script::Attributes::ExcludeFrom))
                {
                    auto excludeAttributeData = azdynamic_cast<const AttributeData<Script::Attributes::ExcludeFlags>*>(attribute);
                    AZ::u64 exclusionFlags = Script::Attributes::ExcludeFlags::List | Script::Attributes::ExcludeFlags::Documentation;
                    bool shouldSkip = (static_cast<AZ::u64>(excludeAttributeData->Get(nullptr)) & exclusionFlags) != 0;
                    if (shouldSkip)
                    {
                        lua_pop(l, 2);
                        continue;
                    }
                }

                lua_rawgeti(l, -2, AZ_LUA_CLASS_METATABLE_NAME_INDEX); // load class name
                AZ_Assert(lua_isstring(l, -1), "Internal scipt error: class without a classname at index %d", AZ_LUA_CLASS_METATABLE_NAME_INDEX);

                AZStd::string classesName = lua_tostring(l, -1);
                classesInfoStr += classesName + " = {\n";
                lua_pop(l, 2); // pop the Class name and behaviorClass
                lua_pushnil(l);
                while (lua_next(l, -2) != 0)
                {
                    if (lua_isstring(l, -2) && lua_isfunction(l, -1))
                    {
                        const char* name = lua_tostring(l, -2);
                        if (lua_tocfunction(l, -1) == &Internal::LuaPropertyTagHelper) // if it's a property
                        {
                            classesInfoStr += GetPropertyString(name, LuaDataType::CLASS);
                        }
                        else
                        {
                            if (strncmp(name, "__", 2) != 0)
                            {
                                classesInfoStr += GetFunctionString(name, LuaDataType::CLASS);
                            }
                        }
                    }
                    lua_pop(l, 1);
                }
                classesInfoStr += "}\n";
            }
            lua_pop(l, 1);
        }
        lua_pop(l, 1); // pop the class table

        WriteStringToFileByType(classesInfoStr.c_str(), LuaDataType::CLASS);
    }

    AZStd::string ScriptAdapterComponent::GetSingleEBusString(BehaviorEBus* ebus)
    {
        bool isHaveEvent = false;
        bool isHaveBroadcast = false;
        bool isHaveNotification = false;
        AZStd::string eventsStr = "";
        AZStd::string broadcasStr = "";
        AZStd::string notificationStr = "";
        AZStd::string ebusStr = "";
        for (const auto& eventSender : ebus->m_events)
        {
            if (eventSender.second.m_event)
            {
                if (!isHaveEvent)
                {
                    eventsStr += "    Event = {\n";
                    isHaveEvent = true;
                }
                AZStd::string funcName = eventSender.first;
                StripQualifiers(funcName);
                eventsStr += GetFunctionString(funcName, LuaDataType::EBUS);
            }
            if (eventSender.second.m_broadcast)
            {
                if (!isHaveBroadcast)
                {
                    broadcasStr += "    Broadcast = {\n";
                    isHaveBroadcast = true;
                }
                AZStd::string funcName = eventSender.first;
                StripQualifiers(funcName);
                broadcasStr += GetFunctionString(funcName, LuaDataType::EBUS);
            }
        }

        if (ebus->m_createHandler)
        {
            BehaviorEBusHandler* handler = nullptr;
            ebus->m_createHandler->InvokeResult(handler);
            if (handler)
            {
                const auto& notifications = handler->GetEvents();
                for (const auto& notification : notifications)
                {
                    if (!isHaveNotification)
                    {
                        notificationStr += "    Notifications = {\n";
                        isHaveNotification = true;
                    }
                    AZStd::string funcName = notification.m_name;
                    StripQualifiers(funcName);
                    notificationStr += GetFunctionString(funcName, LuaDataType::EBUS);
                }
                ebus->m_destroyHandler->Invoke(handler);
            }
        }
        // Concatenate to the final string
        if (isHaveEvent)
        {
            eventsStr += "    },";
            ebusStr += eventsStr;
            if (isHaveBroadcast || isHaveNotification)
            {
                ebusStr += "\n";
            }
        }
        if (isHaveBroadcast)
        {
            broadcasStr += "    },";
            ebusStr += broadcasStr;
            if (isHaveNotification)
            {
                ebusStr += "\n";
            }
        }
        if (isHaveNotification)
        {
            notificationStr += "    },";
            ebusStr += notificationStr;
        }
        return ebusStr;
    }

    void ScriptAdapterComponent::SaveLuaEBusesToFile(ScriptContext* context)
    {
        BehaviorContext* behaviorContext = context->GetBoundContext();
        if (!behaviorContext)
        {
            return;
        }

        AZStd::string ebusesStr = "\n-- ebuses";
        AZStd::vector<AZStd::string> ebusNames;
        for (auto const &ebusPair : behaviorContext->m_ebuses)
        {
            BehaviorEBus* ebus = ebusPair.second;
            // Do not enum if this bus should not be available in Lua
            if (!Internal::IsAvailableInLua(ebus->m_attributes))
            {
                continue;
            }
            // Format ebusName and check whether the ebusName already exists
            AZStd::string ebusName = ebus->m_name;
            FormatEbusName(ebusName);
            bool isFind = false;
            for (AZStd::string* it = ebusNames.begin(); it != ebusNames.end(); ++it)
            {
                if (ebusName == *it)
                {
                    isFind = true;
                    break;
                }
            }
            if (isFind)
            {
                continue;
            }

            ebusNames.push_back(ebusName);
            ebusesStr += "\n" + ebusName + " = {\n";
            ebusesStr += GetSingleEBusString(ebus);
            ebusesStr += "\n}";
        }

        WriteStringToFileByType(ebusesStr.c_str(), LuaDataType::EBUS);
    }

    void ScriptAdapterComponent::SaveLuaFile()
    {
        ScriptContext* scriptContext{};
        ScriptSystemRequestBus::BroadcastResult(scriptContext, &ScriptSystemRequests::GetContext, ScriptContextIds::DefaultScriptContextId);

        //Ensure that the path exists before trying to open the file for writing
        AZStd::string fileDirPath = GetAbsoluteLuaDirPath();
        if (!IO::SystemFile::Exists(fileDirPath.c_str()))
        {
            IO::SystemFile::CreateDir(fileDirPath.c_str());
        }
        else
        {
            SetGenDirWritable(true);
        }

        SaveLuaGlobalsToFile(scriptContext);
        SaveLuaClassesToFile(scriptContext);
        SaveLuaEBusesToFile(scriptContext);
        SetGenDirWritable(false);
    }

    void ScriptAdapterComponent::SetGenDirWritableRecursive(const IO::PathView& path, bool isWritable)
    {
        auto callback = [&path, &isWritable](AZStd::string_view filename, bool isFile) -> bool {
            if (isFile)
            {
                auto filePath = IO::FixedMaxPath(path) / filename;
                IO::SystemFile::SetWritable(filePath.c_str(), isWritable);
            }
            else
            {
                if (filename != "." && filename != "..")
                {
                    auto folderPath = IO::FixedMaxPath(path) / filename;
                    IO::SystemFile::SetWritable(folderPath.c_str(), isWritable);
                    SetGenDirWritableRecursive(folderPath, isWritable);
                }
            }
            return true;
        };
        auto searchPath = IO::FixedMaxPath(path) / "*";
        IO::SystemFile::FindFiles(searchPath.c_str(), callback);
        IO::SystemFile::DeleteDir(IO::FixedMaxPathString(path.Native()).c_str());
    }

    void ScriptAdapterComponent::DeleteFolderRecursive(const IO::PathView& path)
    {
        auto callback = [&path](AZStd::string_view filename, bool isFile) -> bool {
            if (isFile)
            {
                auto filePath = IO::FixedMaxPath(path) / filename;
                IO::SystemFile::Delete(filePath.c_str());
            }
            else
            {
                if (filename != "." && filename != "..")
                {
                    auto folderPath = IO::FixedMaxPath(path) / filename;
                    DeleteFolderRecursive(folderPath);
                }
            }
            return true;
        };
        auto searchPath = IO::FixedMaxPath(path) / "*";
        IO::SystemFile::FindFiles(searchPath.c_str(), callback);
        IO::SystemFile::DeleteDir(IO::FixedMaxPathString(path.Native()).c_str());
    }

    void ScriptAdapterComponent::DeleteLuaFile()
    {
        AZStd::string fileDirPath = GetGenDirPath();
        if (IO::SystemFile::Exists(fileDirPath.c_str()))
        {
            SetGenDirWritable(true);
            IO::PathView pathView(fileDirPath);
            DeleteFolderRecursive(pathView);
        }
    }

}
