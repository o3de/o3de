/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SaveDataSystemComponent.h>
#include <SaveData/SaveDataNotificationBus.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/make_shared.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SaveData
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    const char* SaveDataFileExtension = ".savedata";
    const char* TempSaveDataFileExtension = ".tmpsavedata";

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SaveDataSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<SaveDataSystemComponent>("SaveData", "Provides functionality for saving and loading persistent user data.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("SaveDataService"));
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("SaveDataService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponent::Activate()
    {
        m_pimpl.reset(Implementation::Create(*this));
        SaveDataRequestBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponent::Deactivate()
    {
        SaveDataRequestBus::Handler::BusDisconnect();
        m_pimpl.reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponent::SaveDataBuffer(const SaveDataBufferParams& saveDataBufferParams)
    {
        if (m_pimpl)
        {
            m_pimpl->SaveDataBuffer(saveDataBufferParams);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponent::LoadDataBuffer(const LoadDataBufferParams& loadDataBufferParams)
    {
        if (m_pimpl)
        {
            m_pimpl->LoadDataBuffer(loadDataBufferParams);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponent::SetSaveDataDirectoryPath(const char* saveDataDirectoryPath)
    {
        if (m_pimpl)
        {
            m_pimpl->SetSaveDataDirectoryPath(saveDataDirectoryPath);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SaveDataSystemComponent::Implementation::Implementation(SaveDataSystemComponent& saveDataSystemComponent)
        : m_saveDataSystemComponent(saveDataSystemComponent)
    {
        AZ::TickBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SaveDataSystemComponent::Implementation::~Implementation()
    {
        AZ::TickBus::Handler::BusDisconnect();

        // Make sure we join all active threads, regardless of their completion state.
        JoinAllActiveThreads();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponent::Implementation::OnSaveDataBufferComplete(const AZStd::string& dataBufferName,
                                                                           const AzFramework::LocalUserId localUserId,
                                                                           const SaveDataRequests::OnDataBufferSaved& callback,
                                                                           const SaveDataNotifications::Result& result)
    {
        // Always queue the OnDataBufferSaved notification back on the main thread.
        // Even if this is being called from the main thread already, this ensures
        // the callback / notifications are aways sent at the same time each frame.
        AZ::TickBus::QueueFunction([dataBufferName, localUserId, callback, result]()
        {
            SaveDataNotifications::DataBufferSavedParams dataBufferSavedParams;
            dataBufferSavedParams.dataBufferName = dataBufferName;
            dataBufferSavedParams.localUserId = localUserId;
            dataBufferSavedParams.result = result;
            if (callback)
            {
                callback(dataBufferSavedParams);
            }
            SaveDataNotificationBus::Broadcast(&SaveDataNotifications::OnDataBufferSaved,
                                               dataBufferSavedParams);
        });
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponent::Implementation::SaveDataBufferToFileSystem(const SaveDataBufferParams& saveDataBufferParams,
                                                                             const AZStd::string& absoluteFilePath,
                                                                             bool waitForCompletion,
                                                                             bool useTemporaryFile)
    {
        // Perform parameter error checking but handle gracefully
        AZ_Assert(saveDataBufferParams.dataBuffer, "Invalid param: dataBuffer");
        AZ_Assert(saveDataBufferParams.dataBufferSize, "Invalid param: dataBufferSize");
        AZ_Assert(!saveDataBufferParams.dataBufferName.empty(), "Invalid param: dataBufferName");
        if (!saveDataBufferParams.dataBuffer ||
            !saveDataBufferParams.dataBufferSize ||
            saveDataBufferParams.dataBufferName.empty())
        {
            OnSaveDataBufferComplete(saveDataBufferParams.dataBufferName,
                                     saveDataBufferParams.localUserId,
                                     saveDataBufferParams.callback,
                                     SaveDataNotifications::Result::ErrorInvalid);
            return;
        }

        // Start a new thread to perform the save, capturing the necessary parameters by value,
        // except for the data buffer itself which must be moved (because it is a unique_ptr).
        AZStd::thread_desc saveThreadDesc;
        saveThreadDesc.m_cpuId = AFFINITY_MASK_USERTHREADS;
        saveThreadDesc.m_name = "SaveDataBufferToFileSystem";
        ThreadCompletionPair* threadCompletionPair = nullptr;
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_activeThreadsMutex);
            m_activeThreads.emplace_back();
            threadCompletionPair = &m_activeThreads.back();
        }

        // This is safe access outside the lock guard because we only remove elements from the list
        // after the thread completion flag has been set to true (see also JoinAllCompletedThreads).
        threadCompletionPair->m_thread = AZStd::make_unique<AZStd::thread>(saveThreadDesc,
                                                                           [&threadCompleteFlag = threadCompletionPair->m_threadComplete,
                                                                           dataBuffer = AZStd::move(saveDataBufferParams.dataBuffer),
                                                                           dataBufferSize = saveDataBufferParams.dataBufferSize,
                                                                           dataBufferName = saveDataBufferParams.dataBufferName,
                                                                           onSavedCallback = saveDataBufferParams.callback,
                                                                           localUserId = saveDataBufferParams.localUserId,
                                                                           absoluteFilePath,
                                                                           useTemporaryFile]()
        {
            SaveDataNotifications::Result result = SaveDataNotifications::Result::ErrorUnspecified;

            // If useTemporaryFile == true we save first to a '.tmp' file so we
            // do not overwrite existing save data until we are sure of success.
            const AZStd::string tempSaveDataFilePath = absoluteFilePath + TempSaveDataFileExtension;
            const AZStd::string finalSaveDataFilePath = absoluteFilePath + SaveDataFileExtension;

            // Open the temp save data file for writing, creating it (and
            // any intermediate directories) if it doesn't already exist.
            AZ::IO::SystemFile systemFile;
            const bool openFileResult = systemFile.Open(useTemporaryFile ? tempSaveDataFilePath.c_str() : finalSaveDataFilePath.c_str(),
                                                        AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY |
                                                        AZ::IO::SystemFile::SF_OPEN_CREATE |
                                                        AZ::IO::SystemFile::SF_OPEN_CREATE_PATH);
            if (!openFileResult)
            {
                result = SaveDataNotifications::Result::ErrorIOFailure;
            }
            else
            {
                // Write the data buffer to the temp file and then close it.
                const AZ::IO::SystemFile::SizeType bytesWritten = systemFile.Write(dataBuffer.get(),
                                                                                   dataBufferSize);
                systemFile.Close();

                // Verify that we wrote the correct number of bytes.
                if (bytesWritten != dataBufferSize)
                {
                    result = SaveDataNotifications::Result::ErrorIOFailure;
                }
                else if (useTemporaryFile)
                {
                    // Rename the temp save data file we successfully wrote to.
                    const bool renameFileResult = AZ::IO::SystemFile::Rename(tempSaveDataFilePath.c_str(),
                                                                             finalSaveDataFilePath.c_str(),
                                                                             true);
                    result = renameFileResult ? SaveDataNotifications::Result::Success :
                                                SaveDataNotifications::Result::ErrorIOFailure;

                    // Delete the temp save data file.
                    AZ::IO::SystemFile::Delete(tempSaveDataFilePath.c_str());
                }
                else
                {
                    result = SaveDataNotifications::Result::Success;
                }
            }

            // Invoke the callback and broadcast the OnDataBufferSaved notification from the main thread.
            OnSaveDataBufferComplete(dataBufferName, localUserId, onSavedCallback, result);

            // Set the thread completion flag so it will be joined in JoinAllCompletedThreads.
            threadCompleteFlag = true;
        });

        if (waitForCompletion)
        {
            // The thread completion flag will be set in join, and the thread completion
            // pair removed from m_activeThreads when JoinAllCompletedThreads is called.
            threadCompletionPair->m_thread->join();
            threadCompletionPair->m_thread.reset();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponent::Implementation::OnLoadDataBufferComplete(SaveDataNotifications::DataBuffer dataBuffer,
                                                                           AZ::u64 dataBufferSize,
                                                                           const AZStd::string& dataBufferName,
                                                                           const AzFramework::LocalUserId localUserId,
                                                                           const SaveDataRequests::OnDataBufferLoaded& callback,
                                                                           const SaveDataNotifications::Result& result)
    {
        AZ::TickBus::QueueFunction([dataBuffer, dataBufferSize, dataBufferName, localUserId, callback, result]()
        {
            SaveDataNotifications::DataBufferLoadedParams dataBufferLoadedParams;
            dataBufferLoadedParams.dataBuffer = dataBuffer;
            dataBufferLoadedParams.dataBufferSize = dataBufferSize;
            dataBufferLoadedParams.dataBufferName = dataBufferName;
            dataBufferLoadedParams.localUserId = localUserId;
            dataBufferLoadedParams.result = result;
            if (callback)
            {
                callback(dataBufferLoadedParams);
            }
            SaveDataNotificationBus::Broadcast(&SaveDataNotifications::OnDataBufferLoaded,
                                               dataBufferLoadedParams);
        });
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponent::Implementation::LoadDataBufferFromFileSystem(const LoadDataBufferParams& loadDataBufferParams,
                                                                               const AZStd::string& absoluteFilePath,
                                                                               bool waitForCompletion)
    {
        // Perform parameter error checking but handle gracefully
        AZ_Assert(!loadDataBufferParams.dataBufferName.empty(), "Invalid param: dataBufferName");
        if (loadDataBufferParams.dataBufferName.empty())
        {
            OnLoadDataBufferComplete(nullptr,
                                     0,
                                     loadDataBufferParams.dataBufferName,
                                     loadDataBufferParams.localUserId,
                                     loadDataBufferParams.callback,
                                     SaveDataNotifications::Result::ErrorInvalid);
            return;
        }

        // Start a new thread to perform the load.
        AZStd::thread_desc loadThreadDesc;
        loadThreadDesc.m_cpuId = AFFINITY_MASK_USERTHREADS;
        loadThreadDesc.m_name = "LoadDataBufferFromFileSystem";
        ThreadCompletionPair* threadCompletionPair = nullptr;
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_activeThreadsMutex);
            m_activeThreads.emplace_back();
            threadCompletionPair = &m_activeThreads.back();
        }

        // This is safe access outside the lock guard because we only remove elements from the list
        // after the thread completion flag has been set to true (see also JoinAllCompletedThreads).
        threadCompletionPair->m_thread = AZStd::make_unique<AZStd::thread>(loadThreadDesc,
                                                                           [&threadCompleteFlag = threadCompletionPair->m_threadComplete,
                                                                           loadDataBufferParams,
                                                                           absoluteFilePath]()
        {
            SaveDataNotifications::DataBuffer dataBuffer = nullptr;
            AZ::u64 dataBufferSize = 0;
            SaveDataNotifications::Result result = SaveDataNotifications::Result::ErrorUnspecified;

            // Open the save data file for reading.
            AZ::IO::SystemFile systemFile;
            const AZStd::string finalSaveDataFilePath = absoluteFilePath + SaveDataFileExtension;
            const bool openFileResult = systemFile.Open(finalSaveDataFilePath.c_str(),
                                                        AZ::IO::SystemFile::SF_OPEN_READ_ONLY);
            if (!openFileResult)
            {
                result = SaveDataNotifications::Result::ErrorNotFound;
            }
            else
            {
                // Allocate the memory we'll read the data buffer into.
                // Please note that we use a custom deleter to free it.
                const AZ::IO::SystemFile::SizeType fileLength = systemFile.Length();
                dataBuffer = SaveDataNotifications::DataBuffer(azmalloc(fileLength),
                                                               [](void* p) { azfree(p); });
                if (!dataBuffer)
                {
                    AZ_Error("LoadDataBufferFromFileSystem", false, "Failed to allocate %llu bytes", fileLength);
                    result = SaveDataNotifications::Result::ErrorOutOfMemory;
                }
                else
                {
                    // Read the contents of the file into a data buffer and then close it.
                    dataBufferSize = systemFile.Read(fileLength, dataBuffer.get());
                    systemFile.Close();

                    // Verify that we read the correct number of bytes.
                    result = (dataBufferSize == fileLength) ? SaveDataNotifications::Result::Success :
                                                              SaveDataNotifications::Result::ErrorIOFailure;
                }
            }

            // Invoke the callback and broadcast the OnDataBufferLoaded notification from the main thread.
            OnLoadDataBufferComplete(dataBuffer,
                                     dataBufferSize,
                                     loadDataBufferParams.dataBufferName,
                                     loadDataBufferParams.localUserId,
                                     loadDataBufferParams.callback,
                                     result);

            // Set the thread completion flag so it will be joined in JoinAllCompletedThreads.
            threadCompleteFlag = true;
        });

        if (waitForCompletion)
        {
            // The thread completion flag will be set in join, and the thread completion
            // pair removed from m_activeThreads when JoinAllCompletedThreads is called.
            threadCompletionPair->m_thread->join();
            threadCompletionPair->m_thread.reset();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponent::Implementation::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint scriptTimePoint)
    {
        // We could potentially only do this every n milliseconds, or perhaps try and signal when a
        // thread completes and only check it then, but in almost all cases there will only ever be
        // one save or load thread running at any time (if there are any at all), so iterating over
        // the list each frame to check each atomic bool should not have any impact on performance.
        JoinAllCompletedThreads();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponent::Implementation::JoinAllActiveThreads()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_activeThreadsMutex);
        for (auto& threadCompletionPair : m_activeThreads)
        {
            if (threadCompletionPair.m_thread && threadCompletionPair.m_thread->joinable())
            {
                threadCompletionPair.m_thread->join();
                threadCompletionPair.m_thread.reset();
            }
        }

        // It's important not to call clear (or otherwise modify m_activeThreads) here, but rather
        // only in JoinAllCompletedThreads where we explicitly check for the m_threadComplete flag.
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponent::Implementation::JoinAllCompletedThreads()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_activeThreadsMutex);
        auto it = m_activeThreads.begin();
        while (it != m_activeThreads.end())
        {
            if (it->m_threadComplete)
            {
                if (it->m_thread && it->m_thread->joinable())
                {
                    it->m_thread->join();
                }
                it = m_activeThreads.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}
