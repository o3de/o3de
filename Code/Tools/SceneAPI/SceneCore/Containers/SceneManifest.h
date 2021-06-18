#pragma once

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

#include <stdint.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/document.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/utils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>
#include <SceneAPI/SceneCore/Containers/Views/View.h>
#include <SceneAPI/SceneCore/Containers/Views/ConvertIterator.h>
#include <SceneAPI/SceneCore/DataTypes/IManifestObject.h>

//#define SCENEAPI_SAVE_MANIFEST_AS_XML

namespace AZ
{
    class JsonRegistrationContext;

    namespace SceneAPI
    {
        namespace Containers
        {
            // Scene manifests hold arbitrary meta data about a scene in a dictionary-like fashion.
            //      This can include data such as export groups.
            class SCENE_CORE_API SceneManifest
            {
                friend class SceneManifestContainer;
            public:
                AZ_CLASS_ALLOCATOR_DECL

                AZ_RTTI(SceneManifest, "{9274AD17-3212-4651-9F3B-7DCCB080E467}");
                
                virtual ~SceneManifest();
                
                static AZStd::shared_ptr<const DataTypes::IManifestObject> SceneManifestConstDataConverter(
                    const AZStd::shared_ptr<DataTypes::IManifestObject>& value);

                using Index = size_t;
                static const Index s_invalidIndex = static_cast<Index>(-1);

                using StorageHash = const DataTypes::IManifestObject *;
                using StorageLookup = AZStd::unordered_map<StorageHash, Index>;

                using ValueStorageType = AZStd::shared_ptr<DataTypes::IManifestObject>;
                using ValueStorage = AZStd::vector<ValueStorageType>;
                using ValueStorageData = Views::View<ValueStorage::const_iterator>;

                using ValueStorageConstDataIteratorWrapper = Views::ConvertIterator<ValueStorage::const_iterator,
                            decltype(SceneManifestConstDataConverter(AZStd::shared_ptr<DataTypes::IManifestObject>()))>;
                using ValueStorageConstData = Views::View<ValueStorageConstDataIteratorWrapper>;

                void Clear();
                inline bool IsEmpty() const;

                inline bool AddEntry(const AZStd::shared_ptr<DataTypes::IManifestObject>& value);
                bool AddEntry(AZStd::shared_ptr<DataTypes::IManifestObject>&& value);
                inline bool RemoveEntry(const AZStd::shared_ptr<DataTypes::IManifestObject>& value);
                bool RemoveEntry(const DataTypes::IManifestObject* const value);

                inline size_t GetEntryCount() const;
                inline AZStd::shared_ptr<DataTypes::IManifestObject> GetValue(Index index);
                inline AZStd::shared_ptr<const DataTypes::IManifestObject> GetValue(Index index) const;
                // Finds the index of the given manifest object. A nullptr or invalid object will return s_invalidIndex.
                inline Index FindIndex(const AZStd::shared_ptr<DataTypes::IManifestObject>& value) const;
                // Finds the index of the given manifest object. A nullptr or invalid object will return s_invalidIndex.
                Index FindIndex(const DataTypes::IManifestObject* const value) const;

                inline ValueStorageData GetValueStorage();
                inline ValueStorageConstData GetValueStorage() const;

                bool LoadFromFile(const AZStd::string& absoluteFilePath, SerializeContext* context = nullptr);

                /**
                 * Save manifest to file. Overwrites the file in case it already exists and creates a new file if not.
                 * @param absoluteFilePath the absolute path of the file you want to save to.
                 * @param context If no serialize context was specified, it will get the serialize context from the application component bus.
                 * @result True in case saving went all fine, false if an error occured.
                 */
                bool SaveToFile(const AZStd::string& absoluteFilePath, SerializeContext* context = nullptr);

                AZ::Outcome<void, AZStd::string> LoadFromString(
                    const AZStd::string& fileContents, SerializeContext* context = nullptr,
                    JsonRegistrationContext* registrationContext = nullptr, bool loadXml = false);

                static void Reflect(ReflectContext* context);
                static bool VersionConverter(SerializeContext& context, SerializeContext::DataElementNode& node);

            protected:
                AZ::Outcome<rapidjson::Document, AZStd::string> SaveToJsonDocument(SerializeContext* context = nullptr, JsonRegistrationContext* registrationContext = nullptr);

            private:
                void Init();

#ifdef SCENEAPI_SAVE_MANIFEST_AS_XML
                // If SceneManifest if part of a class normal reflection will work as expected, but if it's directly serialized
                //      additional arguments for the common serialization functions are required. This function is utility uses
                //      the correct arguments to load the SceneManifest from a stream in-place.
                bool LoadFromStream(IO::GenericStream& stream, SerializeContext* context = nullptr);
                // If SceneManifest if part of a class normal reflection will work as expected, but if it's directly serialized
                //      additional arguments for the common serialization functions are required. This function is utility uses
                //      the correct arguments to save the SceneManifest to a stream.
                bool SaveToStream(IO::GenericStream& stream, SerializeContext* context = nullptr) const;

                struct SerializeEvents : public SerializeContext::IEventHandler
                {
                    void OnWriteBegin(void* classPtr) override;
                    void OnWriteEnd(void* classPtr) override;
                };
#endif
                
            AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
                StorageLookup m_storageLookup;
                ValueStorage m_values;
            AZ_POP_DISABLE_OVERRIDE_WARNING
            };
        } // Containers
    } // SceneAPI
} // AZ

#include <SceneAPI/SceneCore/Containers/SceneManifest.inl>
