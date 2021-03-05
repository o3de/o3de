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

#include <SaveData/SaveDataNotificationBus.h>

#include <AzFramework/Input/User/LocalUserId.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SaveData
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to make queries/requests related to saving/loading persistent user data.
    class SaveDataRequests : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can only be sent to and addressed by a single instance (singleton)
        ///@{
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Alias for verbose callback function type
        ///@{
        using OnDataBufferSaved = AZStd::function<void(const SaveDataNotifications::DataBufferSavedParams&)>;
        using OnDataBufferLoaded = AZStd::function<void(const SaveDataNotifications::DataBufferLoadedParams&)>;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! DataBuffer is an alias for the unique_ptr to void saved using a SaveDataBuffer requst.
        //! DataBuffers being saved must have a custom deleter that conforms to DataBufferDeleter.
        //!
        //! DataBufferDeleterAzFree means the buffer will be de-allocated using azfree once it goes
        //! out of scope, meaning it MUST have been allocated in the first place using azmalloc.
        //!
        //! DataBufferDeleterNone means the calling code must delete the data buffer, in which case
        //! it is also responsibile for ensuring it remains valid until the save or load completes.
        //!
        //! If you need to allocate the buffer through some other mechanism but still want it to be
        //! deleted after saved, you can provide a custom deleter conforming to DataBufferDeleter.
        ///@{
        using DataBufferDeleter = void(*)(void*);
        using DataBuffer = AZStd::unique_ptr<void, DataBufferDeleter>;
        static void DataBufferDeleterNone(void*) {}
        static void DataBufferDeleterAzFree(void* ptr)
        {
            azfree(ptr);
        }
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The parameters used to send a save or load serializable object request.
        //! \tparam SerializableType The type of serializable object to save or load.
        template<typename SerializableType>
        struct SaveOrLoadObjectParams
        {
            ////////////////////////////////////////////////////////////////////////////////////////
            //! Alias for verbose callback function type
            using OnObjectSavedOrLoaded = AZStd::function<void(const SaveOrLoadObjectParams&,
                                                               SaveDataNotifications::Result)>;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! A shared ptr to the serializable object to save or load.
            AZStd::shared_ptr<SerializableType> serializableObject;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! The serialize context to use when serializing the object, use nullptr for global one.
            AZ::SerializeContext* serializeContext = nullptr;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! The name of the data buffer to be saved or loaded. Is a filename on most platforms,
            //! but will always uniquely identify the data buffer for the associated local user.
            AZStd::string dataBufferName;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! The local user id the data buffer to be saved or loaded is associated with.
            AzFramework::LocalUserId localUserId = AzFramework::LocalUserIdNone;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Callback function to invoke on the main thread once the object has saved or loaded.
            OnObjectSavedOrLoaded callback = nullptr;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Save a serializable object to persistent storage.
        //! \tparam SerializableType The type of serializable object to save.
        //! \param[in] saveObjectParams The save object request parameters.
        template<typename SerializableType>
        static void SaveObject(const SaveOrLoadObjectParams<SerializableType>& saveObjectParams);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Load a serializable object from persistent storage.
        //! \tparam SerializableType The type of serializable object to load.
        //! \param[in] loadObjectParams The load object request parameters.
        template<typename SerializableType>
        static void LoadObject(const SaveOrLoadObjectParams<SerializableType>& loadObjectParams);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The parameters used to send a save data buffer request.
        struct SaveDataBufferParams
        {
            ////////////////////////////////////////////////////////////////////////////////////////
            //! The data buffer to be saved. Please also see DataBufferDeleter. It is mutable so the
            //! SaveDataBufferParams struct can be passed around by const ref to achieve 'conceptual
            //! constness', but also move-captured by the lambda function that will perform the save.
            mutable DataBuffer dataBuffer = DataBuffer(nullptr, &DataBufferDeleterNone);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! The size of the data buffer to be saved.
            AZ::u64 dataBufferSize = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! The name of the data buffer to be saved. Used as a filename on most platforms, or in
            //! another way to uniquely identify this save data buffer for the associated local user.
            AZStd::string dataBufferName;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! The local user id the data buffer to be saved is associated with.
            AzFramework::LocalUserId localUserId = AzFramework::LocalUserIdNone;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Callback function to invoke on the main thread once the data buffer has been saved.
            OnDataBufferSaved callback = nullptr;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The parameters used to send a load data buffer request.
        struct LoadDataBufferParams
        {
            ////////////////////////////////////////////////////////////////////////////////////////
            //! The name of the data buffer to be loaded. Used as a filename on most platforms or in
            //! another way to uniquely identify this save data buffer for the associated local user.
            AZStd::string dataBufferName;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! The local user id the data buffer to be loaded is associated with.
            AzFramework::LocalUserId localUserId = AzFramework::LocalUserIdNone;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Callback function to invoke on the main thread once the data buffer has been loaded.
            OnDataBufferLoaded callback = nullptr;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Save a data buffer to persistent storage.
        //! \param[in] saveDataBufferRequestParams The save data buffer request parameters.
        virtual void SaveDataBuffer(const SaveDataBufferParams& saveDataBufferParams) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Load a data buffer from persistent storage.
        //! \param[in] loadDataBufferParams The load data buffer request parameters.
        virtual void LoadDataBuffer(const LoadDataBufferParams& loadDataBufferParams) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Set the path to the application's save data dircetory. If the supplied path is absolute,
        //! it will be used directy, otherwise if it's relative it will be appended to the location
        //! deemed most appropriate by the host OS for storing application specific user save data.
        //!
        //! If this is never called, save data will be saved in and loaded from a directory with the
        //! same name as the executable, relative to the default location for storing user save data.
        //!
        //! One some systems (ie. consoles), the location of save data is fixed and/or inaccessible
        //! using the standard file-system, in which case calling this function will have no effect.
        //!
        //! But on systems where we are able to override the default save data directory path, care
        //! should be taken that it is only done once at startup before any attempt to load or save.
        //!
        //! \param[in] saveDataDirectoryPath The new path to the application's save data dircetory.
        virtual void SetSaveDataDirectoryPath(const char* saveDataDirectoryPath) = 0;
    };
    using SaveDataRequestBus = AZ::EBus<SaveDataRequests>;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<class SerializableType>
    inline void SaveDataRequests::SaveObject(const SaveOrLoadObjectParams<SerializableType>& saveObjectParams)
    {
        // Save the serializable object to a data buffer.
        AZStd::vector<AZ::u8> dataBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> dataStream(&dataBuffer);
        const bool saved = AZ::Utils::SaveObjectToStream(dataStream,
                                                         AZ::ObjectStream::ST_BINARY,
                                                         saveObjectParams.serializableObject.get(),
                                                         saveObjectParams.serializeContext);
        if (!saved)
        {
            AZ_Error("SaveDataRequests::SaveObject", false,
                     "Failed to save serializable object to data stream.");
            if (saveObjectParams.callback)
            {
                saveObjectParams.callback(saveObjectParams, SaveDataNotifications::Result::ErrorCorrupt);
            }
            return;
        }

        // Save the data buffer to persistent storage.
        const AZ::u64 dataBufferSize = dataBuffer.size();
        SaveDataBufferParams saveDataBufferParams;
        if (dataBufferSize)
        {
            saveDataBufferParams.dataBuffer = DataBuffer(azmalloc(dataBufferSize), DataBufferDeleterAzFree);
            memcpy(saveDataBufferParams.dataBuffer.get(), dataBuffer.data(), dataBufferSize);
        }
        saveDataBufferParams.dataBufferSize = dataBufferSize;
        saveDataBufferParams.dataBufferName = saveObjectParams.dataBufferName;
        saveDataBufferParams.localUserId = saveObjectParams.localUserId;
        saveDataBufferParams.callback = [saveObjectParams](const SaveDataNotifications::DataBufferSavedParams& dataBufferSavedParams)
        {
            if (saveObjectParams.callback)
            {
                saveObjectParams.callback(saveObjectParams, dataBufferSavedParams.result);
            }
        };
        SaveDataRequestBus::Broadcast(&SaveDataRequests::SaveDataBuffer, saveDataBufferParams);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<class SerializableType>
    inline void SaveDataRequests::LoadObject(const SaveOrLoadObjectParams<SerializableType>& loadObjectParams)
    {
        // Load the data buffer from persistent storage.
        LoadDataBufferParams loadDataBufferParams;
        loadDataBufferParams.dataBufferName = loadObjectParams.dataBufferName;
        loadDataBufferParams.localUserId = loadObjectParams.localUserId;
        loadDataBufferParams.callback = [loadObjectParams](const SaveDataNotifications::DataBufferLoadedParams& dataBufferLoadedParams)
        {
            SaveDataNotifications::Result result = dataBufferLoadedParams.result;
            if (result == SaveDataNotifications::Result::Success)
            {
                // Load the serializable object from the data buffer.
                const bool loaded = AZ::Utils::LoadObjectFromBufferInPlace(dataBufferLoadedParams.dataBuffer.get(),
                                                                           dataBufferLoadedParams.dataBufferSize,
                                                                           *(loadObjectParams.serializableObject),
                                                                           loadObjectParams.serializeContext);
                if (!loaded)
                {
                    AZ_Error("SaveDataRequests::LoadObject", loaded,
                             "Failed to load serializable object from data stream.");
                    result = SaveDataNotifications::Result::ErrorCorrupt;
                }
            }

            if (loadObjectParams.callback)
            {
                loadObjectParams.callback(loadObjectParams, result);
            }
        };
        SaveDataRequestBus::Broadcast(&SaveDataRequests::LoadDataBuffer, loadDataBufferParams);
    }
} // namespace SaveData
