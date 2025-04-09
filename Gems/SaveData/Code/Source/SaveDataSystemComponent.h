/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <SaveData/SaveDataRequestBus.h>
#include <SaveData_Traits_Platform.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SaveData
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! A system component providing functionality related to saving / loading persistent user data.
    class SaveDataSystemComponent : public AZ::Component
                                  , public SaveDataRequestBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::Component Setup
        AZ_COMPONENT(SaveDataSystemComponent, "{35790061-347E-47F1-B803-9523752ECD39}");

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::ComponentDescriptor::Reflect
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::ComponentDescriptor::GetProvidedServices
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::ComponentDescriptor::GetIncompatibleServices
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default constructor
        SaveDataSystemComponent() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~SaveDataSystemComponent() override = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::Component::Activate
        void Activate() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::Component::Deactivate
        void Deactivate() override;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref SaveData::SaveDataRequests::SaveDataBuffer
        void SaveDataBuffer(const SaveDataBufferParams& saveDataBufferParams) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref SaveData::SaveDataRequests::LoadDataBuffer
        void LoadDataBuffer(const LoadDataBufferParams& loadDataBufferParams) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref SaveData::SaveDataRequests::SetSaveDataDirectoryPath
        void SetSaveDataDirectoryPath(const char* saveDataDirectoryPath) override;

    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Base class for platform specific implementations of the save data system component
        class Implementation : public AZ::TickBus::Handler
        {
        public:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Allocator
            AZ_CLASS_ALLOCATOR(Implementation, AZ::SystemAllocator);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Default factory create function
            //! \param[in] saveDataSystemComponent Reference to the parent being implemented
            static Implementation* Create(SaveDataSystemComponent& saveDataSystemComponent);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Constructor
            //! \param[in] saveDataSystemComponent Reference to the parent being implemented
            Implementation(SaveDataSystemComponent& saveDataSystemComponent);

            ////////////////////////////////////////////////////////////////////////////////////////
            // Disable copying
            AZ_DISABLE_COPY_MOVE(Implementation);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Default destructor
            virtual ~Implementation();

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Save a data buffer.
            //! \param[in] saveDataBufferRequestParams The save data buffer request parameters.
            virtual void SaveDataBuffer(const SaveDataBufferParams& saveDataBufferParams) = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Load a data buffer.
            //! \param[in] loadDataBufferParams The load data buffer request parameters.
            virtual void LoadDataBuffer(const LoadDataBufferParams& loadDataBufferParams) = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Set the path to the application's save data dircetory. Does nothing on some systems.
            //! \param[in] saveDataDirectoryPath The path to the application's save data dircetory.
            virtual void SetSaveDataDirectoryPath(const char* saveDataDirectoryPath) = 0;

        protected:
            ////////////////////////////////////////////////////////////////////////////////////////
            //! Convenience function to broadcast SaveDataNotifications::OnDataBufferSaved events in
            //! addition to any callback specified when SaveDataRequests::SaveDataBuffer was called.
            //! \param[in] dataBufferName The name of the data buffer that was saved.
            //! \param[in] localUserId The local user id the data that was saved is associated with.
            //! \param[in] result The result of the save data buffer request.
            //! \param[in] callback The data buffer saved callback to invoke.
            static void OnSaveDataBufferComplete(const AZStd::string& dataBufferName,
                                                 const AzFramework::LocalUserId localUserId,
                                                 const SaveDataRequests::OnDataBufferSaved& callback,
                                                 const SaveDataNotifications::Result& result);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Save a data buffer to the file system.
            //! \param[in] saveDataBufferRequestParams The save data buffer request parameters.
            //! \param[in] absoluteFilePath The absolute file path where to save the data buffer.
            //! \param[in] waitForCompletion Should we wait until the save data thread completes?
            //! \param[in] useTemporaryFile Should we write to a temporary file that gets renamed?
            void SaveDataBufferToFileSystem(const SaveDataBufferParams& saveDataBufferParams,
                                            const AZStd::string& absoluteFilePath,
                                            bool waitForCompletion = false,
                                            bool useTemporaryFile = true);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Convenience function to broadcast SaveDataNotifications::OnDataBufferLoaded events in
            //! addition to any callback specified when SaveDataRequests::LoadDataBuffer was called.
            //! \param[in] dataBuffer The data buffer that was loaded.
            //! \param[in] dataBufferSize The size of the data buffer that was loaded.
            //! \param[in] dataBufferName The name of the data buffer that was loaded.
            //! \param[in] localUserId The local user id the data that was loaded is associated with.
            //! \param[in] result The result of the load data buffer request.
            //! \param[in] callback The data buffer loaded callback to invoke.
            static void OnLoadDataBufferComplete(SaveDataNotifications::DataBuffer dataBuffer,
                                                 AZ::u64 dataBufferSize,
                                                 const AZStd::string& dataBufferName,
                                                 const AzFramework::LocalUserId localUserId,
                                                 const SaveDataRequests::OnDataBufferLoaded& callback,
                                                 const SaveDataNotifications::Result& result);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Load a data buffer from the file system.
            //! \param[in] loadDataBufferParams The load data buffer request parameters.
            //! \param[in] absoluteFilePath The absolute file path from where to load the data buffer.
            //! \param[in] waitForCompletion Should we wait until the load data thread completes?
            void LoadDataBufferFromFileSystem(const LoadDataBufferParams& loadDataBufferParams,
                                              const AZStd::string& absoluteFilePath,
                                              bool waitForCompletion = false);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Pairing of a save/load thread with an atomic bool indicating whether it is complete
            struct ThreadCompletionPair
            {
                AZStd::unique_ptr<AZStd::thread> m_thread;
                AZStd::atomic_bool m_threadComplete{ false };
            };

            ////////////////////////////////////////////////////////////////////////////////////////
            //! \ref AZ::TickEvents::OnTick
            void OnTick(float deltaTime, AZ::ScriptTimePoint scriptTimePoint) override;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Convenience function to join all threads that are active
            void JoinAllActiveThreads();

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Convenience function to join all threads that have been marked as completed
            void JoinAllCompletedThreads();

            ////////////////////////////////////////////////////////////////////////////////////////
            // Variables
            AZStd::mutex m_activeThreadsMutex; //! Mutex to restrict access to the active threads
            AZStd::list<ThreadCompletionPair> m_activeThreads; //!< A container of active threads
            SaveDataSystemComponent& m_saveDataSystemComponent; //!< Reference to the parent
        };

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Private pointer to the platform specific implementation
        AZStd::unique_ptr<Implementation> m_pimpl;
    };
}
