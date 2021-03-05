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

#include <AzFramework/Input/User/LocalUserId.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SaveData
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to listen for notifications related to the saving of persistent user data.
    class SaveDataNotifications : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: save data notifications are addressed to a single address
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: save data notifications can be handled by multiple listeners
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! DataBuffer is an alias for the shared_ptr to void loaded using a LoadDataBuffer request.
        //! Unlike SaveDataRequests::DataBuffer (a unique_ptr), SaveDataNotifications::DataBuffer is
        //! a shared_ptr so that listeners can decide whether they want/need to hold onto the memory.
        using DataBuffer = AZStd::shared_ptr<void>;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Enum representing the result of a save or load data buffer request.
        enum class Result
        {
            Success,          //!< The save/load data buffer request was successful.
            ErrorCanceled,    //!< The save/load data buffer request failed: user cancelled.
            ErrorCorrupt,     //!< The save/load data buffer request failed: buffer corrupt.
            ErrorInvalid,     //!< The save/load data buffer request failed: invalid params.
            ErrorNotFound,    //!< The save/load data buffer request failed: file not found.
            ErrorIOFailure,   //!< The save/load data buffer request failed: file IO failure.
            ErrorInProgress,  //!< The save/load data buffer request failed: already in progress.
            ErrorOutOfMemory, //!< The save/load data buffer request failed: insufficient memory.
            ErrorSyncFailure, //!< The save/load data buffer request failed: synchronization issue.
            ErrorUnknownUser, //!< The save/load data buffer request failed: local user id unknown.
            ErrorUnspecified  //!< The save/load data buffer request failed: reason is unspecified.
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The parameters sent with a data buffer saved notification.
        struct DataBufferSavedParams
        {
            ////////////////////////////////////////////////////////////////////////////////////////
            //! The name of the data buffer that was saved. Used as a filename on most platforms, or
            //! in another way to uniquely identify this save data buffer for the associated user id.
            AZStd::string dataBufferName;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! The local user id the data buffer that was saved is associated with.
            AzFramework::LocalUserId localUserId = AzFramework::LocalUserIdNone;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! The result of the save data buffer request.
            Result result = Result::Success;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The parameters sent with a data buffer loaded notification.
        struct DataBufferLoadedParams
        {
            ////////////////////////////////////////////////////////////////////////////////////////
            //! The data buffer that was loaded.
            DataBuffer dataBuffer = nullptr;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! The size of the data buffer that was loaded.
            AZ::u64 dataBufferSize = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! The name of the data buffer that was loaded. Used as a filename on most platforms or
            //! in another way to uniquely identify this save data buffer for the associated user id.
            AZStd::string dataBufferName;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! The local user id the data buffer that was loaded is associated with.
            AzFramework::LocalUserId localUserId = AzFramework::LocalUserIdNone;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! The result of the load data buffer request.
            Result result = Result::Success;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when a data buffer save has completed, successfully or otherwise.
        //! Will always be broadcast from the main thread.
        //! \param[in] dataBufferSavedParams The data buffer saved notification parameters.
        virtual void OnDataBufferSaved(const DataBufferSavedParams& dataBufferSavedParams) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when a data buffer load has completed, successfully or otherwise.
        //! Will always be broadcast from the main thread.
        //! \param[in] dataBufferLoadedParams The data buffer loaded notification parameters.
        virtual void OnDataBufferLoaded(const DataBufferLoadedParams& dataBufferLoadedParams) = 0;
    };
    using SaveDataNotificationBus = AZ::EBus<SaveDataNotifications>;
} // namespace SaveData
