/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/JSON/document.h>
#include <AzCore/JSON/pointer.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/scoped_lock.h>

namespace AZ
{
    class StackedString;
    struct JsonImportSettings;

    class SettingsRegistryImpl final
        : public SettingsRegistryInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(SettingsRegistryImpl, AZ::OSAllocator);
        AZ_RTTI(AZ::SettingsRegistryImpl, "{E9C34190-F888-48CA-83C9-9F24B4E21D72}", AZ::SettingsRegistryInterface);

        static constexpr size_t MaxRegistryFolderEntries = 128;
        
        SettingsRegistryImpl();
        //! @param useFileIo - If true attempt to redirect
        //! file read operations through the FileIOBase instance first before falling back to SystemFile
        //! otherwise always use SystemFile
        explicit SettingsRegistryImpl(bool useFileIo);
        AZ_DISABLE_COPY_MOVE(SettingsRegistryImpl);
        ~SettingsRegistryImpl() override;

        void SetContext(SerializeContext* context);
        void SetContext(JsonRegistrationContext* context);
        
        [[nodiscard]] SettingsType GetType(AZStd::string_view path) const override;
        bool Visit(Visitor& visitor, AZStd::string_view path) const override;
        bool Visit(const VisitorCallback& callback, AZStd::string_view path) const override;
        [[nodiscard]] NotifyEventHandler RegisterNotifier(NotifyCallback callback) override;
        void RegisterNotifier(NotifyEventHandler& hanlder) override;
        void ClearNotifiers();

        [[nodiscard]] PreMergeEventHandler RegisterPreMergeEvent(PreMergeEventCallback callback) override;
        void RegisterPreMergeEvent(PreMergeEventHandler& handler) override;
        [[nodiscard]] PostMergeEventHandler RegisterPostMergeEvent(PostMergeEventCallback callback) override;
        void RegisterPostMergeEvent(PostMergeEventHandler& handler) override;
        void ClearMergeEvents();

        bool Get(bool& result, AZStd::string_view path) const override;
        bool Get(s64& result, AZStd::string_view path) const override;
        bool Get(u64& result, AZStd::string_view path) const override;
        bool Get(double& result, AZStd::string_view path) const override;
        bool Get(AZStd::string& result, AZStd::string_view path) const override;
        bool Get(SettingsRegistryInterface::FixedValueString& result, AZStd::string_view path) const override;
        bool GetObject(void* result, AZ::Uuid resultTypeID, AZStd::string_view path) const override;

        bool Set(AZStd::string_view path, bool value) override;
        bool Set(AZStd::string_view path, s64 value) override;
        bool Set(AZStd::string_view path, u64 value) override;
        bool Set(AZStd::string_view path, double value) override;
        bool Set(AZStd::string_view path, AZStd::string_view value) override;
        bool Set(AZStd::string_view path, const char* value) override;
        bool SetObject(AZStd::string_view path, const void* value, AZ::Uuid valueTypeID) override;

        bool Remove(AZStd::string_view path) override;

        bool MergeCommandLineArgument(AZStd::string_view argument, AZStd::string_view anchorKey,
            const CommandLineArgumentSettings& commandLineSettings) override;
        MergeSettingsResult MergeSettings(AZStd::string_view data, Format format, AZStd::string_view anchorKey = "") override;
        MergeSettingsResult MergeSettingsFile(AZStd::string_view path, Format format, AZStd::string_view anchorKey = "",
            AZStd::vector<char>* scratchBuffer = nullptr) override;
        MergeSettingsResult MergeSettingsFolder(AZStd::string_view path, const Specializations& specializations,
            AZStd::string_view platform, AZStd::string_view anchorKey = "", AZStd::vector<char>* scratchBuffer = nullptr) override;

        void SetNotifyForMergeOperations(bool notify) override;
        bool GetNotifyForMergeOperations() const override;

        void SetUseFileIO(bool useFileIo) override;

    private:
        using TagList = AZStd::fixed_vector<size_t, Specializations::MaxCount + 1>;
        struct RegistryFile
        {
            AZ::IO::FixedMaxPathString m_relativePath;
            TagList m_tags;
            bool m_isPatch{ false };
            bool m_isPlatformFile{ false };
        };
        using RegistryFileList = AZStd::fixed_vector<RegistryFile, MaxRegistryFolderEntries>;

        [[nodiscard]] SettingsType GetTypeNoLock(AZStd::string_view path) const;

        template<typename T>
        bool SetValueInternal(AZStd::string_view path, T value);
        template<typename T>
        bool GetValueInternal(T& result, AZStd::string_view path) const;
        VisitResponse Visit(Visitor& visitor, StackedString& path, AZStd::string_view valueName,
            const rapidjson::Value& value) const;

        // Compares if lhs is less than rhs in terms of processing order. This can also detect and report conflicts.
        bool IsLessThan(MergeSettingsResult& collisionFoundResult, const RegistryFile& lhs, const RegistryFile& rhs, const Specializations& specializations,
            AZStd::string_view folderPath);
        bool ExtractFileDescription(RegistryFile& output, AZStd::string_view filename, const Specializations& specializations);
        MergeSettingsResult MergeSettingsFileInternal(const char* path, Format format, AZStd::string_view rootKey);
        MergeSettingsResult MergeSettingsJsonDocument(const rapidjson::Document& jsonPatch, Format format, AZStd::string_view rootKey,
            AZ::IO::PathView filePath);

        //! The filePath here is for the loaded json content in the string parameter
        //! The jsonData parameter is accepted by value for efficiency as the string
        //! will be modified inside of the method
        MergeSettingsResult MergeSettingsString(AZStd::string jsonData, Format format, AZStd::string_view anchorKey,
            AZ::IO::PathView filePath);
        MergeSettingsResult LoadJsonFileIntoString(AZStd::string& jsonData, const char* filePath);

        void SignalNotifier(AZStd::string_view jsonPath, SettingsType type);

        //! Locks the m_settingMutex but also checks to make sure that someone is not currently
        //! visiting/iterating over the registry, which is invalid if you're about to modify it
        AZStd::scoped_lock<AZStd::recursive_mutex> LockForWriting() const;

        //! For symmetry with the above, locks with intent to only read data.  This can be done
        //! even during iteration/visiting.
        AZStd::scoped_lock<AZStd::recursive_mutex> LockForReading() const;

        // only use the setting mutex via the above functions.
        mutable AZStd::recursive_mutex m_settingMutex;
        mutable AZStd::recursive_mutex m_notifierMutex;
        NotifyEvent m_notifiers;
        PreMergeEvent m_preMergeEvent;
        PostMergeEvent m_postMergeEvent;

        //! NOTE: During SignalNotifier, the registered notify event handlers are moved to a local NotifyEvent
        //! Therefore setting a value within the registry during signaling will queue future SignalNotifier calls
        //! These calls will then be invoked after the current signaling has completed
        //! This is done to avoid deadlock if another thread attempts to access register a notifier or signal one
        mutable AZStd::mutex m_signalMutex;
        struct SignalNotifierArgs
        {
            FixedValueString m_jsonPath;
            SettingsType m_type;
            AZ::IO::FixedMaxPath m_mergeFilePath;
        };
        AZStd::deque<SignalNotifierArgs> m_signalNotifierQueue;
        AZStd::atomic_int m_signalCount{};

        rapidjson::Document m_settings;
        JsonSerializerSettings m_serializationSettings;
        JsonDeserializerSettings m_deserializationSettings;
        //! If set to true, then the JSON Patch/JSON Merge Patch operations
        //! also result in the signaling of a notification event
        bool m_mergeOperationNotify{};
        //! When true use the Registered FileIOBase for file open operations
        bool m_useFileIo{};


        struct ScopedMergeEvent
        {
            ScopedMergeEvent(SettingsRegistryImpl& settingsRegistry, MergeEventArgs mergeEventArgs);
            ~ScopedMergeEvent();

            SettingsRegistryImpl& m_settingsRegistry;
            MergeEventArgs m_mergeEventArgs;
        };
        //! Stack tracking the files currently being merged
        //! This is protected by m_settingsMutex
        AZStd::stack<AZ::IO::FixedMaxPath> m_mergeFilePathStack;

        // if this is nonzero, we are in a visit operation.  It can be used to detect illegal modifications
        // of the tree during visit.
        mutable int m_visitDepth = 0; // mutable due to it being a debugging value used in const.

    };
} // namespace AZ
