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

#include <cctype>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/JSON/error/en.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/parallel/scoped_lock.h>

namespace AZ
{
    template<typename T>
    bool SettingsRegistryImpl::SetValueInternal(AZStd::string_view path, T value, SettingsRegistryInterface::Type type)
    {
        if (path.empty())
        {
            // rapidjson::Pointer assets that the supplied string
            // is not nullptr even if the supplied size is 0
            // Setting to empty string to prevent assert
            path = "";
        }
        rapidjson::Pointer pointer(path.data(), path.length());
        if (pointer.IsValid())
        {
            if constexpr (AZStd::is_same_v<T, bool> || AZStd::is_same_v<T, double>)
            {
                pointer.Set(m_settings, value);
            }
            else if constexpr (AZStd::is_same_v<T, s64>)
            {
                rapidjson::Value& setting = pointer.Create(m_settings, m_settings.GetAllocator());
                setting.SetInt64(value);
            }
            else if constexpr (AZStd::is_same_v<T, u64>)
            {
                rapidjson::Value& setting = pointer.Create(m_settings, m_settings.GetAllocator());
                setting.SetUint64(value);
            }
            else if constexpr (AZStd::is_same_v<T, AZStd::string_view>)
            {
                rapidjson::Value& setting = pointer.Create(m_settings, m_settings.GetAllocator());
                setting.SetString(value.data(), aznumeric_caster(value.length()), m_settings.GetAllocator());
            }
            else
            {
                static_assert(!AZStd::is_same_v<T, T>, "SettingsRegistryImpl::SetValueInternal called with unsupported type.");
            }

            m_notifiers.Signal(path, type);
            return true;
        }
        return false;
    }

    template<typename T>
    bool SettingsRegistryImpl::GetValueInternal(T& result, AZStd::string_view path) const
    {
        if (path.empty())
        {
            // rapidjson::Pointer assets that the supplied string
            // is not nullptr even if the supplied size is 0
            // Setting to empty string to prevent assert
            path = "";
        }
        rapidjson::Pointer pointer(path.data(), path.length());
        if (pointer.IsValid())
        {
            const rapidjson::Value* value = pointer.Get(m_settings);
            if constexpr (AZStd::is_same_v<T, bool>)
            {
                if (value && value->IsBool())
                {
                    result = value->GetBool();
                    return true;
                }
            }
            else if constexpr (AZStd::is_same_v<T, s64>)
            {
                if (value && value->IsInt64())
                {
                    result = value->GetInt64();
                    return true;
                }
            }
            else if constexpr (AZStd::is_same_v<T, u64>)
            {
                if (value && value->IsUint64())
                {
                    result = value->GetUint64();
                    return true;
                }
            }
            else if constexpr (AZStd::is_same_v<T, double>)
            {
                if (value && value->IsDouble())
                {
                    result = value->GetDouble();
                    return true;
                }
            }
            else if constexpr (AZStd::is_same_v<T, AZStd::string> || AZStd::is_same_v<T, SettingsRegistryInterface::FixedValueString>)
            {
                if (value && value->IsString())
                {
                    result.append(value->GetString(), value->GetStringLength());
                    return true;
                }
            }
            else
            {
                static_assert(!AZStd::is_same_v<T,T>, "SettingsRegistryImpl::GetValueInternal called with unsupported type.");
            }
        }
        return false;
    }

    SettingsRegistryImpl::SettingsRegistryImpl()
    {
        m_serializationSettings.m_keepDefaults = true;

        rapidjson::Pointer pointer(AZ_SETTINGS_REGISTRY_HISTORY_KEY);
        pointer.Create(m_settings, m_settings.GetAllocator()).SetArray();
    }

    void SettingsRegistryImpl::SetContext(SerializeContext* context)
    {
        AZStd::scoped_lock lock(m_settingMutex);

        m_serializationSettings.m_serializeContext = context;
        m_deserializationSettings.m_serializeContext = context;
    }

    void SettingsRegistryImpl::SetContext(JsonRegistrationContext* context)
    {
        AZStd::scoped_lock lock(m_settingMutex);

        m_serializationSettings.m_registrationContext = context;
        m_deserializationSettings.m_registrationContext = context;
    }

    bool SettingsRegistryImpl::Visit(Visitor& visitor, AZStd::string_view path) const
    {
        if (path.empty())
        {
            // rapidjson::Pointer assets that the supplied string
            // is not nullptr even if the supplied size is 0
            // Setting to empty string to prevent assert
            path = "";
        }
        AZStd::scoped_lock lock(m_settingMutex);

        rapidjson::Pointer pointer(path.data(), path.length());
        if (pointer.IsValid())
        {
            const rapidjson::Value* value = pointer.Get(m_settings);
            if (value)
            {
                StackedString jsonPath(StackedString::Format::JsonPointer);
                if (!path.empty())
                {
                    path.remove_prefix(1); // Remove the leading slash as the StackedString will add this back in.
                    jsonPath.Push(path);
                }
                Visit(visitor, jsonPath, "", *value);
                return true;
            }
        }
        return false;
    }

    bool SettingsRegistryImpl::Visit(const VisitorCallback& callback, AZStd::string_view path) const
    {
        struct CallbackVisitor : public Visitor
        {
            explicit CallbackVisitor(const VisitorCallback& callback) : m_callback(callback) {};

            VisitResponse Traverse(AZStd::string_view path, AZStd::string_view valueName, VisitAction action, Type type) override
            {
                return m_callback(path, valueName, action, type);
            }

            const VisitorCallback& m_callback;
        };
        CallbackVisitor visitor(callback);
        return Visit(visitor, path);
    }

    auto SettingsRegistryImpl::RegisterNotifier(const NotifyCallback& callback) -> NotifyEventHandler
    {
        NotifyEventHandler notifyHandler{ callback };
        {
            AZStd::scoped_lock lock(m_settingMutex);
            notifyHandler.Connect(m_notifiers);
        }
        return notifyHandler;
    }

    auto SettingsRegistryImpl::RegisterNotifier(NotifyCallback&& callback) -> NotifyEventHandler
    {
        NotifyEventHandler notifyHandler{ AZStd::move(callback) };
        {
            AZStd::scoped_lock lock(m_settingMutex);
            notifyHandler.Connect(m_notifiers);
        }
        return notifyHandler;
    }

    void SettingsRegistryImpl::ClearNotifiers()
    {
        AZStd::scoped_lock lock(m_settingMutex);
        m_notifiers.DisconnectAllHandlers();
    }

    SettingsRegistryInterface::Type SettingsRegistryImpl::GetType(AZStd::string_view path) const
    {
        if (path.empty())
        {
            //rapidjson::Pointer assets that the supplied string
            // is not nullptr even if the supplied size is 0
            // Setting to empty string to prevent assert
            path = "";
        }

        AZStd::scoped_lock lock(m_settingMutex);

        rapidjson::Pointer pointer(path.data(), path.length());
        if (pointer.IsValid())
        {
            const rapidjson::Value* value = pointer.Get(m_settings);
            if (value)
            {
                switch (value->GetType())
                {
                case rapidjson::Type::kNullType:
                    return Type::Null;
                case rapidjson::Type::kFalseType:
                    return Type::Boolean;
                case rapidjson::Type::kTrueType:
                    return Type::Boolean;
                case rapidjson::Type::kObjectType:
                    return Type::Object;
                case rapidjson::Type::kArrayType:
                    return Type::Array;
                case rapidjson::Type::kStringType:
                    return Type::String;
                case rapidjson::Type::kNumberType:
                    return 
                        value->IsDouble() ? Type::FloatingPoint :
                        Type::Integer;
                }
            }
        }
        return Type::NoType;
    }

    bool SettingsRegistryImpl::Get(bool& result, AZStd::string_view path) const
    {
        AZStd::scoped_lock lock(m_settingMutex);
        return GetValueInternal(result, path);
    }

    bool SettingsRegistryImpl::Get(s64& result, AZStd::string_view path) const
    {
        AZStd::scoped_lock lock(m_settingMutex);
        return GetValueInternal(result, path);
    }

    bool SettingsRegistryImpl::Get(u64& result, AZStd::string_view path) const
    {
        AZStd::scoped_lock lock(m_settingMutex);
        return GetValueInternal(result, path);
    }

    bool SettingsRegistryImpl::Get(double& result, AZStd::string_view path) const
    {
        AZStd::scoped_lock lock(m_settingMutex);
        return GetValueInternal(result, path);
    }

    bool SettingsRegistryImpl::Get(AZStd::string& result, AZStd::string_view path) const
    {
        AZStd::scoped_lock lock(m_settingMutex);
        return GetValueInternal(result, path);
    }

    bool SettingsRegistryImpl::Get(FixedValueString& result, AZStd::string_view path) const
    {
        AZStd::scoped_lock lock(m_settingMutex);
        return GetValueInternal(result, path);
    }

    bool SettingsRegistryImpl::GetObject(void* result, Uuid resultTypeID, AZStd::string_view path) const
    {
        if (path.empty())
        {
            // rapidjson::Pointer assets that the supplied string
            // is not nullptr even if the supplied size is 0
            // Setting to empty string to prevent assert
            path = "";
        }
        AZStd::scoped_lock lock(m_settingMutex);

        rapidjson::Pointer pointer(path.data(), path.length());
        if (pointer.IsValid())
        {
            const rapidjson::Value* value = pointer.Get(m_settings);
            if (value)
            {
                JsonSerializationResult::ResultCode jsonResult = JsonSerialization::Load(result, resultTypeID, *value, m_deserializationSettings);
                return jsonResult.GetProcessing() != JsonSerializationResult::Processing::Halted;
            }
        }
        return false;
    }

    bool SettingsRegistryImpl::Set(AZStd::string_view path, bool value)
    {
        AZStd::scoped_lock lock(m_settingMutex);
        return SetValueInternal(path, value, Type::Boolean);
    }

    bool SettingsRegistryImpl::Set(AZStd::string_view path, s64 value)
    {
        AZStd::scoped_lock lock(m_settingMutex);
        return SetValueInternal(path, value, Type::Integer);
    }

    bool SettingsRegistryImpl::Set(AZStd::string_view path, u64 value)
    {
        AZStd::scoped_lock lock(m_settingMutex);
        return SetValueInternal(path, value, Type::Integer);
    }

    bool SettingsRegistryImpl::Set(AZStd::string_view path, double value)
    {
        AZStd::scoped_lock lock(m_settingMutex);
        return SetValueInternal(path, value, Type::FloatingPoint);
    }

    bool SettingsRegistryImpl::Set(AZStd::string_view path, AZStd::string_view value)
    {
        AZStd::scoped_lock lock(m_settingMutex);
        return SetValueInternal(path, value, Type::String);
    }

    bool SettingsRegistryImpl::Set(AZStd::string_view path, const char* value)
    {
        return Set(path, AZStd::string_view{ value });
    }

    bool SettingsRegistryImpl::SetObject(AZStd::string_view path, const void* value, Uuid valueTypeID)
    {
        if (path.empty())
        {
            //rapidjson::Pointer assets that the supplied string
            // is not nullptr even if the supplied size is 0
            // Setting to empty string to prevent assert
            path = "";
        }

        AZStd::scoped_lock lock(m_settingMutex);

        rapidjson::Pointer pointer(path.data(), path.length());
        if (pointer.IsValid())
        {
            rapidjson::Value store;
            JsonSerializationResult::ResultCode jsonResult = JsonSerialization::Store(store, m_settings.GetAllocator(), 
                value, nullptr, valueTypeID, m_serializationSettings);
            if (jsonResult.GetProcessing() != JsonSerializationResult::Processing::Halted)
            {
                rapidjson::Value& setting = pointer.Create(m_settings, m_settings.GetAllocator());
                setting = AZStd::move(store);
                m_notifiers.Signal(path, Type::Object);
                return true;
            }
        }
        return false;
    }

    bool SettingsRegistryImpl::Remove(AZStd::string_view path)
    {
        if (path.empty())
        {
            // rapidjson::Pointer assets that the supplied string
            // is not nullptr even if the supplied size is 0
            // Setting to empty string to prevent assert
            path = "";
        }
        AZStd::scoped_lock lock(m_settingMutex);
        rapidjson::Pointer pointerPath(path.data(), path.size());
        if (!pointerPath.IsValid())
        {
            return false;
        }

        return pointerPath.Erase(m_settings);
    }

    bool SettingsRegistryImpl::MergeCommandLineArgument(AZStd::string_view argument, AZStd::string_view rootKey,
        const CommandLineArgumentSettings& commandLineSettings)
    {
        const char* front = argument.begin();
        const char* back = argument.end();

        if (!commandLineSettings.m_delimiterFunc)
        {
            AZ_Error("SettingsRegistry", false,
                R"(No delimiter function, therefore there splitting of the argument "%.*s" into a key value pair cannot be done)",
                aznumeric_cast<int>(argument.size()), argument.data());
            return false;
        }
        const char* split = AZStd::find_if(front, back, commandLineSettings.m_delimiterFunc);
        if (split == front || // There is no key
            split == (back-1) || // There is no value
            split == back) // Split character not found. 
        {
            return false;
        }

        const char* keyStart = front;
        while (std::isspace(*keyStart)) // This is safe because it will eventually stop on =
        {
            keyStart++;
        }
        if (keyStart == split) // Key is just white spaces
        {
            return false;
        }
        const char* keyEnd = split;
        while (std::isspace(*--keyEnd));
        keyEnd++;

        char buffer[MaxJsonPathLength];
        AZStd::string_view key;
        bool keyHasDivider = *keyStart == '/';
        if (!rootKey.empty())
        {
            bool rootKeyHasDivider = (rootKey[rootKey.length() - 1]) == '/';
            size_t count;
            if (!rootKeyHasDivider && !keyHasDivider)
            {
                count = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "%.*s/%.*s",
                    aznumeric_cast<int>(rootKey.length()), rootKey.data(),
                    aznumeric_cast<int>(keyEnd - keyStart), keyStart);
            }
            else if (rootKeyHasDivider && keyHasDivider)
            {
                count = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "%.*s%.*s",
                    aznumeric_cast<int>(rootKey.length()) - 1, rootKey.data(),
                    aznumeric_cast<int>(keyEnd - keyStart), keyStart);
            }
            else
            {
                count = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "%.*s%.*s",
                    aznumeric_cast<int>(rootKey.length()), rootKey.data(),
                    aznumeric_cast<int>(keyEnd - keyStart), keyStart);
            }
            if (count >= AZ_ARRAY_SIZE(buffer) - 1)
            {
                return false;
            }
            key = AZStd::string_view(buffer, count);
        }
        else if (!keyHasDivider)
        {
            size_t count = azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "/%.*s",
                aznumeric_cast<int>(keyEnd - keyStart), keyStart);
            if (count >= AZ_ARRAY_SIZE(buffer) - 1)
            {
                return false;
            }
            key = AZStd::string_view(buffer, count);
        }
        else
        {
            key = AZStd::string_view(keyStart, keyEnd);
        }

        const char* valueStart = split + 1;
        while (std::isspace(*valueStart) && valueStart < back)
        {
            valueStart++;
        }
        if (valueStart == back)
        {
            return false; // The value is empty
        }
        const char* valueEnd = back;
        while (std::isspace(*(--valueEnd)));
        valueEnd++;

        AZStd::string_view value(valueStart, valueEnd);
        if (value == "true")
        {
            return Set(key, true);
        }
        if (value == "false")
        {
            return Set(key, false);
        }

        if (value.length() - 1 >= MaxCommandLineArgumentLength)
        {
            return false;
        }
        char argumentString[MaxCommandLineArgumentLength];
        snprintf(argumentString, AZ_ARRAY_SIZE(argument), "%.*s", aznumeric_cast<int>(value.length()), value.data());
        char* argumentStringEnd = argumentString + value.length();

        char* convertEnd = nullptr;
        s64 intValue = strtoll(argumentString, &convertEnd, 0);
        if (convertEnd == argumentStringEnd)
        {
            return Set(key, intValue);
        }
        convertEnd = nullptr;
        double floatingPointValue = strtod(argumentString, &convertEnd);
        if (convertEnd == argumentStringEnd)
        {
            return Set(key, floatingPointValue);
        }

        // If the type isn't a boolean, integer or floating point then treat the value as a string.
        return Set(key, value);
    }

    bool SettingsRegistryImpl::MergeSettings(AZStd::string_view data, Format format)
    {
        rapidjson::Document jsonPatch;
        constexpr int flags = rapidjson::kParseStopWhenDoneFlag | rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag;
        jsonPatch.Parse<flags>(data.data(), data.length());
        if (jsonPatch.HasParseError())
        {
            AZ_Error("Settings Registry", false, R"(Unable to parse data due to json error "%s" at offset %llu.)",
                GetParseError_En(jsonPatch.GetParseError()), jsonPatch.GetErrorOffset());
            return false;
        }

        JsonMergeApproach mergeApproach;
        switch (format)
        {
        case Format::JsonPatch:
            mergeApproach = JsonMergeApproach::JsonPatch;
            break;
        case Format::JsonMergePatch:
            mergeApproach = JsonMergeApproach::JsonMergePatch;
            break;
        default:
            AZ_Assert(false, "Provided format for merging settings into the Setting Registry is unsupported.");
            return false;
        }

        AZStd::scoped_lock lock(m_settingMutex);

        JsonSerializationResult::ResultCode mergeResult =
            JsonSerialization::ApplyPatch(m_settings, m_settings.GetAllocator(), jsonPatch, mergeApproach);
        if (mergeResult.GetProcessing() != JsonSerializationResult::Processing::Completed)
        {
            AZ_Error("Settings Registry", false, "Failed to fully merge data into registry.");
            return false;
        }

        m_notifiers.Signal("", Type::Object);

        return true;
    }

    bool SettingsRegistryImpl::MergeSettingsFile(AZStd::string_view path, Format format, AZStd::string_view rootKey,
        AZStd::vector<char>* scratchBuffer)
    {
        using namespace rapidjson;

        if (path.empty())
        {
            AZ_Error("Settings Registry", false, "Path provided for MergeSettingsFile is empty.");
            return false;
        }

        AZStd::vector<char> buffer;
        if (!scratchBuffer)
        {
            scratchBuffer = &buffer;
        }

        AZStd::scoped_lock lock(m_settingMutex);

        bool result = false;
        if (path[path.length()] == 0)
        {
            result = MergeSettingsFileInternal(path.data(), format, rootKey, *scratchBuffer);
        }
        else
        {
            if (MaxFilePathLength < path.length() + 1)
            {
                AZ_Error("Settings Registry", false,
                    R"(Path "%.*s" is too long. Either make sure that the provided path is terminated or use a shorter path.)",
                    static_cast<int>(path.length()), path.data());
                Pointer pointer(AZ_SETTINGS_REGISTRY_HISTORY_KEY "/-");
                Value pathValue(path.data(), aznumeric_caster(path.length()), m_settings.GetAllocator());
                pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
                    .AddMember(StringRef("Error"), StringRef("Unable to read registry file."), m_settings.GetAllocator())
                    .AddMember(StringRef("Path"), AZStd::move(pathValue), m_settings.GetAllocator());
                return false;
            }
            char filePath[MaxFilePathLength];
            azstrncpy(filePath, AZ_ARRAY_SIZE(filePath), path.data(), path.length());
            filePath[path.length()] = 0;
            result = MergeSettingsFileInternal(filePath, format, rootKey, *scratchBuffer);
        }

        scratchBuffer->clear();
        return result;
    }

    bool SettingsRegistryImpl::MergeSettingsFolder(AZStd::string_view path, const Specializations& specializations,
        AZStd::string_view platform, AZStd::string_view rootKey, AZStd::vector<char>* scratchBuffer)
    {
        using namespace AZ::IO;
        using namespace rapidjson;

        if (path.empty())
        {
            AZ_Error("Settings Registry", false, "Path provided for MergeSettingsFolder is empty.");
            return false;
        }


        AZStd::vector<char> buffer;
        if (!scratchBuffer)
        {
            scratchBuffer = &buffer;
        }

        Pointer pointer(AZ_SETTINGS_REGISTRY_HISTORY_KEY "/-");

        size_t additionalSpaceRequired = 3; // 3 is for the '/', '*' and 0
        if (!platform.empty())
        {
            additionalSpaceRequired += AZ_ARRAY_SIZE(PlatformFolder) + platform.length() + 2; // +2 for the two slashes.
        }

        if (path.length() + additionalSpaceRequired > MaxFilePathLength)
        {
            AZ_Error("Settings Registry", false, "Folder path for the Setting Registry is too long: %.*s",
                static_cast<int>(path.size()), path.data());
            pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
                .AddMember(StringRef("Error"), StringRef("Folder path for the Setting Registry is too long."), m_settings.GetAllocator())
                .AddMember(StringRef("Path"), Value(path.data(), aznumeric_caster(path.length()), m_settings.GetAllocator()), m_settings.GetAllocator());
            return false;
        }

        RegistryFileList fileList;
        scratchBuffer->clear();

        AZStd::fixed_string<MaxFilePathLength> folderPath{ path };
        constexpr AZStd::string_view pathSeparators{ AZ_CORRECT_AND_WRONG_DATABASE_SEPARATOR };
        if (pathSeparators.find_first_of(folderPath.back()) == AZStd::string_view::npos)
        {
            folderPath.push_back(AZ_CORRECT_DATABASE_SEPARATOR);
        }

        const size_t platformKeyOffset = folderPath.size();
        folderPath.push_back('*');

        Value specialzationArray(kArrayType);
        size_t specializationCount = specializations.GetCount();
        for (size_t i = 0; i < specializationCount; ++i)
        {
            AZStd::string_view name = specializations.GetSpecialization(i);
            specialzationArray.PushBack(Value(name.data(), aznumeric_caster(name.length()), m_settings.GetAllocator()), m_settings.GetAllocator());
        }
        pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
            .AddMember(StringRef("Folder"), Value(folderPath.c_str(), aznumeric_caster(folderPath.size()), m_settings.GetAllocator()), m_settings.GetAllocator())
            .AddMember(StringRef("Specializations"), AZStd::move(specialzationArray), m_settings.GetAllocator());

        auto callback = [this, &fileList, &specializations, &pointer, &folderPath](const char* filename, bool isFile) -> bool
        {
            if (isFile)
            {
                if (fileList.size() >= MaxRegistryFolderEntries)
                {
                    AZ_Error("Settings Registry", false, "Too many files in registry folder.");
                    pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
                        .AddMember(StringRef("Error"), StringRef("Too many files in registry folder."), m_settings.GetAllocator())
                        .AddMember(StringRef("Path"), Value(folderPath.c_str(), aznumeric_caster(folderPath.size()), m_settings.GetAllocator()), m_settings.GetAllocator())
                        .AddMember(StringRef("File"), Value(filename, m_settings.GetAllocator()), m_settings.GetAllocator());
                    return false;
                }

                fileList.push_back();
                RegistryFile& registryFile = fileList.back();
                if (!ExtractFileDescription(registryFile, filename, specializations))
                {
                    fileList.pop_back();
                }
            }
            return true;
        };
        SystemFile::FindFiles(folderPath.c_str(), callback);


        AZStd::scoped_lock lock(m_settingMutex);
        if (!platform.empty())
        {
            // Move the folderPath prefix back to the supplied path before the wildcard
            folderPath.erase(platformKeyOffset);
            folderPath += PlatformFolder;
            folderPath.push_back(AZ_CORRECT_DATABASE_SEPARATOR);
            folderPath += platform;
            folderPath.push_back(AZ_CORRECT_DATABASE_SEPARATOR);
            folderPath.push_back('*');

            auto platformCallback = [this, &fileList, &specializations, &pointer, &folderPath](const char* filename, bool isFile) -> bool
            {
                if (isFile)
                {
                    if (fileList.size() >= MaxRegistryFolderEntries)
                    {
                        AZ_Error("Settings Registry", false, "Too many files in registry folder.");
                        pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
                            .AddMember(StringRef("Error"), StringRef("Too many files in registry folder."), m_settings.GetAllocator())
                            .AddMember(StringRef("Path"), Value(folderPath.c_str(), aznumeric_caster(folderPath.size()), m_settings.GetAllocator()), m_settings.GetAllocator())
                            .AddMember(StringRef("File"), Value(filename, m_settings.GetAllocator()), m_settings.GetAllocator());
                        return false;
                    }

                    fileList.push_back();
                    RegistryFile& registryFile = fileList.back();
                    if (ExtractFileDescription(registryFile, filename, specializations))
                    {
                        registryFile.m_isPlatformFile = true;
                    }
                    else
                    {
                        fileList.pop_back();
                    }
                }
                return true;
            };
            SystemFile::FindFiles(folderPath.c_str(), platformCallback);
        }

        if (!fileList.empty())
        {
            // Sort the registry files in the order
            bool collisionFound = false;
            auto sorter = [this, &collisionFound, &specializations, &pointer, path](
                const RegistryFile& lhs, const RegistryFile& rhs) -> bool
            {
                return IsLessThan(collisionFound, lhs, rhs, specializations, pointer, path);
            };
            AZStd::sort(fileList.begin(), fileList.end(), sorter);

            if (collisionFound)
            {
                return false;
            }

            // Load the registry files in the sorted order.
            for (RegistryFile& registryFile : fileList)
            {
                folderPath.erase(platformKeyOffset); // Erase all characters after the platformKeyOffset
                if (registryFile.m_isPlatformFile)
                {
                    folderPath += PlatformFolder;
                    folderPath.push_back(AZ_CORRECT_DATABASE_SEPARATOR);
                    folderPath += platform;
                    folderPath.push_back(AZ_CORRECT_DATABASE_SEPARATOR);
                }

                folderPath += registryFile.m_relativePath;

                if (!registryFile.m_isPatch)
                {
                    MergeSettingsFileInternal(folderPath.c_str(), Format::JsonMergePatch, rootKey, *scratchBuffer);
                }
                else
                {
                    MergeSettingsFileInternal(folderPath.c_str(), Format::JsonPatch, rootKey, *scratchBuffer);
                }
                scratchBuffer->clear();
            }
        }
        return true;
    }

    SettingsRegistryInterface::VisitResponse SettingsRegistryImpl::Visit(Visitor& visitor, StackedString& path, AZStd::string_view valueName,
        const rapidjson::Value& value) const
    {
        VisitResponse result;
        switch (value.GetType())
        {
        case rapidjson::Type::kNullType:
            result = visitor.Traverse(path, valueName, VisitAction::Value, Type::Null);
            break;
        case rapidjson::Type::kFalseType:
            result = visitor.Traverse(path, valueName, VisitAction::Value, Type::Boolean);
            if (result == VisitResponse::Continue)
            {
                visitor.Visit(path, valueName, Type::Boolean, false);
            }
            break;
        case rapidjson::Type::kTrueType:
            result = visitor.Traverse(path, valueName, VisitAction::Value, Type::Boolean);
            if (result == VisitResponse::Continue)
            {
                visitor.Visit(path, valueName, Type::Boolean, true);
            }
            break;
        case rapidjson::Type::kObjectType:
            result = visitor.Traverse(path, valueName, VisitAction::Begin, Type::Object);
            if (result == VisitResponse::Continue)
            {
                for (const auto& member : value.GetObject())
                {
                    AZStd::string_view fieldName(member.name.GetString(), member.name.GetStringLength());
                    path.Push(fieldName);
                    if (Visit(visitor, path, fieldName, member.value) == VisitResponse::Done)
                    {
                        return VisitResponse::Done;
                    }
                    path.Pop();
                }
                if (visitor.Traverse(path, valueName, VisitAction::End, Type::Object) == VisitResponse::Done)
                {
                    return VisitResponse::Done;
                }
            }
            break;
        case rapidjson::Type::kArrayType:
            result = visitor.Traverse(path, valueName, VisitAction::Begin, Type::Array);
            if (result == VisitResponse::Continue)
            {
                size_t counter = 0;
                for (const rapidjson::Value& entry : value.GetArray())
                {
                    size_t endIndex = path.Get().size();
                    path.Push(counter);
                    // Push can reallocate the internal AZ::OSString on the StackedString, invalidating existing iterators
                    // Therefore StackedString::Get() is called again to retrieve a fresh string_view
                    AZStd::string_view entryName(path.Get());
                    entryName.remove_prefix(endIndex + 1);
                    if (Visit(visitor, path, entryName, entry) == VisitResponse::Done)
                    {
                        return VisitResponse::Done;
                    }
                    counter++;
                    path.Pop();
                }
                if (visitor.Traverse(path, valueName, VisitAction::End, Type::Array) == VisitResponse::Done)
                {
                    return VisitResponse::Done;
                }
            }
            break;
        case rapidjson::Type::kStringType:
            result = visitor.Traverse(path, valueName, VisitAction::Value, Type::String);
            if (result == VisitResponse::Continue)
            {
                visitor.Visit(path, valueName, Type::String, AZStd::string_view(value.GetString(), value.GetStringLength()));
            }
            break;
        case rapidjson::Type::kNumberType:
            if (value.IsDouble())
            {
                result = visitor.Traverse(path, valueName, VisitAction::Value, Type::FloatingPoint);
                if (result == VisitResponse::Continue)
                {
                    visitor.Visit(path, valueName, Type::FloatingPoint, value.GetDouble());
                }
            }
            else
            {
                result = visitor.Traverse(path, valueName, VisitAction::Value, Type::Integer);
                if (result == VisitResponse::Continue)
                {
                    if (value.IsInt64())
                    {
                        s64 integerValue = value.GetInt64();
                        visitor.Visit(path, valueName, Type::Integer, integerValue);
                    }
                    else
                    {
                        u64 integerValue = value.GetUint64();
                        visitor.Visit(path, valueName, Type::Integer, integerValue);
                    }
                }
            }
            break;
        default:
            AZ_Assert(false, "Unsupported RapidJSON type: %i.", aznumeric_cast<int>(value.GetType()));
            result = VisitResponse::Done;
        }
        return result;
    }

    bool SettingsRegistryImpl::IsLessThan(bool& collisionFound, const RegistryFile& lhs, const RegistryFile& rhs,
        const Specializations& specializations, const rapidjson::Pointer& historyPointer, AZStd::string_view folderPath)
    {
        using namespace rapidjson;

        AZ_Assert(!lhs.m_tags.empty(), "Comparing a settings file without at least a name tag.");
        AZ_Assert(!rhs.m_tags.empty(), "Comparing a settings file without at least a name tag.");

        // Sort by the name first so the registry file gets applied with all its specializations.
        if (lhs.m_tags[0] != rhs.m_tags[0])
        {
            return strcmp(lhs.m_relativePath, rhs.m_relativePath) < 0;
        }

        // Then sort by size first so the files with the fewest specializations get applied first.
        const size_t lhsTagsSize = lhs.m_tags.size();
        const size_t rhsTagsSize = rhs.m_tags.size();
        if (lhsTagsSize != rhsTagsSize)
        {
            return lhsTagsSize < rhsTagsSize;
        }

        // If registry files have the same number of tags then sort them by the order they appear
        // in the specialization list.
        const size_t numTags = lhsTagsSize; // Pick either lhs or rhs as they're the same length.
        for (size_t i = 1; i < numTags; ++i)
        {
            const size_t left = specializations.GetPriority(lhs.m_tags[i]);
            const size_t right = specializations.GetPriority(rhs.m_tags[i]);
            if (left != right)
            {
                return left < right;
            }
        }

        if (lhs.m_isPlatformFile != rhs.m_isPlatformFile)
        {
            return !lhs.m_isPlatformFile;
        }

        collisionFound = true;
        AZ_Error("Settings Registry", false, R"(Two registry files point to the same specialization: "%s" and "%s")",
            lhs.m_relativePath, rhs.m_relativePath);
        historyPointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
            .AddMember(StringRef("Error"), StringRef("Too many files in registry folder."), m_settings.GetAllocator())
            .AddMember(StringRef("Path"),
                Value(folderPath.data(), aznumeric_caster(folderPath.length()), m_settings.GetAllocator()), m_settings.GetAllocator())
            .AddMember(StringRef("File1"), Value(lhs.m_relativePath, m_settings.GetAllocator()), m_settings.GetAllocator())
            .AddMember(StringRef("File2"), Value(rhs.m_relativePath, m_settings.GetAllocator()), m_settings.GetAllocator());
        return false;
    }

    bool SettingsRegistryImpl::ExtractFileDescription(RegistryFile& output, const char* filename, const Specializations& specializations)
    {
        if (!filename || filename[0] == 0)
        {
            AZ_Error("Settings Registry", false, "Settings file without name found");
            return false;
        }

        AZStd::string_view filePath{ filename };
        const size_t filePathSize = filePath.size();

        // The filePath.empty() check makes sure that the file extension after the final <dot> isn't added to the output.m_tags
        AZStd::optional<AZStd::string_view> pathTag = AZ::StringFunc::TokenizeNext(filePath, '.');
        for (; pathTag && !filePath.empty(); pathTag = AZ::StringFunc::TokenizeNext(filePath, '.'))
        {
            output.m_tags.push_back(Specializations::Hash(*pathTag));
        }

        // If token is invalid, then the filename has no <dot> characters and therefore no extension
        if (pathTag)
        {
            if (pathTag->size() >= AZStd::char_traits<char>::length(PatchExtension) && azstrnicmp(pathTag->data(), PatchExtension, pathTag->size()) == 0)
            {
                output.m_isPatch = true;
            }
            else if (pathTag->size() != AZStd::char_traits<char>::length(Extension) || azstrnicmp(pathTag->data(), Extension, pathTag->size()) != 0)
            {
                return false;
            }
        }
        else
        {
            AZ_Error("Settings Registry", false, R"(Settings file without extension found: "%s")", filename);
            return false;
        }

        if (output.m_tags.size() > 1)
        {
            // Remove files with duplicate tags. This is considered an error.
            AZStd::sort(AZStd::next(output.m_tags.begin()), output.m_tags.end());
            TagList::iterator currentIt = output.m_tags.begin();
            TagList::iterator endIt = output.m_tags.end();
            ++currentIt;
            while (currentIt != endIt)
            {
                if (*currentIt == *(currentIt - 1))
                {
                    AZ_Error("Settings Registry", false, R"(One or more tags are duplicated in registry file "%s")", filename);
                    return false;
                }
                ++currentIt;
            }

            // Remove registry file if it won't be applied because there's a specialization that's not been selected. Note that
            // the first tag is actually the hash of the filename for later use so will be skipped.
            size_t tagCount = output.m_tags.size();
            for (size_t i = 1; i < tagCount; ++i)
            {
                if (!specializations.Contains(output.m_tags[i]))
                {
                    return false;
                }
            }
        }

        // Sort the specialization tags so they're in a consistent order for quicker compares later on. Skip the first one as
        // thats the name tag.
        AZStd::sort(AZStd::next(output.m_tags.begin()), output.m_tags.end());

        if (filePathSize < AZ_ARRAY_SIZE(output.m_relativePath))
        {
            azstrcpy(output.m_relativePath, AZ_ARRAY_SIZE(output.m_relativePath), filename);
            return true;
        }
        else
        {
            AZ_Error("Settings Registry", false, R"(Found relative path to settings file "%s" is too long.)", filename);
            return false;
        }
    }

    bool SettingsRegistryImpl::MergeSettingsFileInternal(const char* path, Format format, AZStd::string_view rootKey,
        AZStd::vector<char>& scratchBuffer)
    {
        using namespace AZ::IO;
        using namespace rapidjson;

        Pointer pointer(AZ_SETTINGS_REGISTRY_HISTORY_KEY "/-");

        SystemFile file;
        if (!file.Open(path, SystemFile::OpenMode::SF_OPEN_READ_ONLY))
        {
            AZ_Error("Settings Registry", false, R"(Unable to open registry file "%s".)", path);
            pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
                .AddMember(StringRef("Error"), StringRef("Unable to open registry file."), m_settings.GetAllocator())
                .AddMember(StringRef("Path"), Value(path, m_settings.GetAllocator()), m_settings.GetAllocator());
            return false;
        }

        u64 fileSize = file.Length();
        scratchBuffer.clear();
        scratchBuffer.resize_no_construct(fileSize + 1);
        if (file.Read(fileSize, scratchBuffer.data()) != fileSize)
        {
            AZ_Error("Settings Registry", false, R"(Unable to read registry file "%s".)", path);
            pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
                .AddMember(StringRef("Error"), StringRef("Unable to read registry file."), m_settings.GetAllocator())
                .AddMember(StringRef("Path"), Value(path, m_settings.GetAllocator()), m_settings.GetAllocator());
            return false;
        }
        scratchBuffer[fileSize] = 0;

        rapidjson::Document jsonPatch;
        constexpr int flags = rapidjson::kParseStopWhenDoneFlag | rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag;
        jsonPatch.ParseInsitu<flags>(scratchBuffer.data());
        if (jsonPatch.HasParseError())
        {
            AZ_Error("Settings Registry", false, R"(Unable to parse registry file "%s" due to json error "%s" at offset %llu.)",
                path, GetParseError_En(jsonPatch.GetParseError()), jsonPatch.GetErrorOffset());
            pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
                .AddMember(StringRef("Error"), StringRef("Unable to parse registry file due to invalid json."), m_settings.GetAllocator())
                .AddMember(StringRef("Path"), Value(path, m_settings.GetAllocator()), m_settings.GetAllocator())
                .AddMember(StringRef("Message"), StringRef(GetParseError_En(jsonPatch.GetParseError())), m_settings.GetAllocator())
                .AddMember(StringRef("Offset"), aznumeric_cast<uint64_t>(jsonPatch.GetErrorOffset()), m_settings.GetAllocator());
            return false;
        }

        JsonMergeApproach mergeApproach;
        switch (format)
        {
        case Format::JsonPatch:
            mergeApproach = JsonMergeApproach::JsonPatch;
            break;
        case Format::JsonMergePatch:
            mergeApproach = JsonMergeApproach::JsonMergePatch;
            if (!jsonPatch.IsObject())
            {
                AZ_Error("Settings Registry", false, R"(Attempting to merge the settings registry file "%s" where the root element is a)"
                    R"( non-JSON Object using the JSON MergePatch approach. The JSON MergePatch algorithm would therefore)"
                    R"( overwrite all settings at the supplied root-key path and therefore merging has been)"
                    R"( disallowed to prevent field destruction.)" "\n"
                    R"(To merge the supplied settings registry file, the settings within it must be placed within a JSON Object '{}')"
                    R"( in order to allow moving of its fields using the root-key as an anchor.)", path);

                pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
                    .AddMember(StringRef("Error"), StringRef("Cannot merge registry file with a root which is not a JSON Object,"
                        " an empty root key and a merge approach of JsonMergePatch. Otherwise the Settings Registry would be overridden."
                        " See RFC 7386 for more information"), m_settings.GetAllocator())
                    .AddMember(StringRef("Path"), Value(path, m_settings.GetAllocator()), m_settings.GetAllocator());
                return false;
            }
            break;
        default:
            AZ_Assert(false, "Provided format for merging settings into the Setting Registry is unsupported.");
            return false;
        }

        JsonSerializationResult::ResultCode mergeResult(JsonSerializationResult::Tasks::Merge);
        if (rootKey.empty())
        {
            mergeResult = JsonSerialization::ApplyPatch(m_settings, m_settings.GetAllocator(), jsonPatch, mergeApproach);
        }
        else
        {
            Pointer root(rootKey.data(), rootKey.length());
            if (root.IsValid())
            {
                Value& rootValue = root.Create(m_settings, m_settings.GetAllocator());
                mergeResult = JsonSerialization::ApplyPatch(rootValue, m_settings.GetAllocator(), jsonPatch, mergeApproach);
            }
            else
            {
                AZ_Error("Settings Registry", false, R"(Failed to root path "%.*s" is invalid.)",
                    aznumeric_cast<int>(rootKey.length()), rootKey.data());
                pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
                    .AddMember(StringRef("Error"), StringRef("Invalid root key."), m_settings.GetAllocator())
                    .AddMember(StringRef("Path"), Value(path, m_settings.GetAllocator()), m_settings.GetAllocator());
                return false;
            }
        }
        if (mergeResult.GetProcessing() != JsonSerializationResult::Processing::Completed)
        {
            AZ_Error("Settings Registry", false, R"(Failed to fully merge registry file "%s".)", path);
            pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
                .AddMember(StringRef("Error"), StringRef("Failed to fully merge registry file."), m_settings.GetAllocator())
                .AddMember(StringRef("Path"), Value(path, m_settings.GetAllocator()), m_settings.GetAllocator());
            return false;
        }

        pointer.Create(m_settings, m_settings.GetAllocator()).SetString(path, m_settings.GetAllocator());

        m_notifiers.Signal("", Type::Object);

        return true;
    }
} // namespace AZ
