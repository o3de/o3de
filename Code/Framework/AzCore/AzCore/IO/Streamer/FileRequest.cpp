/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <limits>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/IO/Streamer/RequestPath.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/StreamerContext.h>

namespace AZ::IO
{
    //
    // Command structures.
    //

    FileRequest::ExternalRequestData::ExternalRequestData(FileRequestPtr&& request)
        : m_request(AZStd::move(request))
    {}

    FileRequest::RequestPathStoreData::RequestPathStoreData(RequestPath path)
        : m_path(AZStd::move(path))
    {}

    FileRequest::ReadRequestData::ReadRequestData(RequestPath path, void* output, u64 outputSize, u64 offset, u64 size,
        AZStd::chrono::system_clock::time_point deadline, IStreamerTypes::Priority priority)
        : m_path(AZStd::move(path))
        , m_allocator(nullptr)
        , m_deadline(deadline)
        , m_output(output)
        , m_outputSize(outputSize)
        , m_offset(offset)
        , m_size(size)
        , m_priority(priority)
        , m_memoryType(IStreamerTypes::MemoryType::ReadWrite) // Only generic memory can be assigned externally.
    {}

    FileRequest::ReadRequestData::ReadRequestData(RequestPath path, IStreamerTypes::RequestMemoryAllocator* allocator,
        u64 offset, u64 size, AZStd::chrono::system_clock::time_point deadline, IStreamerTypes::Priority priority)
        : m_path(AZStd::move(path))
        , m_allocator(allocator)
        , m_deadline(deadline)
        , m_output(nullptr)
        , m_outputSize(0)
        , m_offset(offset)
        , m_size(size)
        , m_priority(priority)
        , m_memoryType(IStreamerTypes::MemoryType::ReadWrite) // Only generic memory can be assigned externally.
    {}

    FileRequest::ReadRequestData::~ReadRequestData()
    {
        if (m_allocator != nullptr)
        {
            if (m_output != nullptr)
            {
                m_allocator->Release(m_output);
            }
            m_allocator->UnlockAllocator();
        }
    }

    FileRequest::ReadData::ReadData(void* output, u64 outputSize, const RequestPath& path, u64 offset, u64 size, bool sharedRead)
        : m_output(output)
        , m_outputSize(outputSize)
        , m_path(path)
        , m_offset(offset)
        , m_size(size)
        , m_sharedRead(sharedRead)
    {}

    FileRequest::CompressedReadData::CompressedReadData(CompressionInfo&& compressionInfo, void* output, u64 readOffset, u64 readSize)
        : m_compressionInfo(AZStd::move(compressionInfo))
        , m_output(output)
        , m_readOffset(readOffset)
        , m_readSize(readSize)
    {}

    FileRequest::FileExistsCheckData::FileExistsCheckData(const RequestPath& path)
        : m_path(path)
    {}

    FileRequest::FileMetaDataRetrievalData::FileMetaDataRetrievalData(const RequestPath& path)
        : m_path(path)
    {}

    FileRequest::CancelData::CancelData(FileRequestPtr target)
        : m_target(AZStd::move(target))
    {}

    FileRequest::FlushData::FlushData(RequestPath path)
        : m_path(AZStd::move(path))
    {}

    FileRequest::RescheduleData::RescheduleData(FileRequestPtr target, AZStd::chrono::system_clock::time_point newDeadline,
        IStreamerTypes::Priority newPriority)
        : m_target(AZStd::move(target))
        , m_newDeadline(newDeadline)
        , m_newPriority(newPriority)
    {}

    FileRequest::CreateDedicatedCacheData::CreateDedicatedCacheData(RequestPath path, const FileRange& range)
        : m_path(AZStd::move(path))
        , m_range(range)
    {}

    FileRequest::DestroyDedicatedCacheData::DestroyDedicatedCacheData(RequestPath path, const FileRange& range)
        : m_path(AZStd::move(path))
        , m_range(range)
    {}

    FileRequest::ReportData::ReportData(ReportType reportType)
        : m_reportType(reportType)
    {}

    FileRequest::CustomData::CustomData(AZStd::any data, bool failWhenUnhandled)
        : m_data(AZStd::move(data))
        , m_failWhenUnhandled(failWhenUnhandled)
    {}


    //
    // FileRequest
    //

    FileRequest::FileRequest(Usage usage)
        : m_usage(usage)
    {
        Reset();
    }

    FileRequest::~FileRequest()
    {
        Reset();
    }

    void FileRequest::CreateRequestLink(FileRequestPtr&& request)
    {
        AZ_Assert(AZStd::holds_alternative<AZStd::monostate>(m_command),
            "Attempting to set FileRequest to 'RequestLink', but another task was already assigned.");
        m_parent = request->m_request.m_parent;
        request->m_request.m_parent = this;
        m_dependencies++;
        m_command.emplace<ExternalRequestData>(AZStd::move(request));
    }

    void FileRequest::CreateRequestPathStore(FileRequest* parent, RequestPath path)
    {
        AZ_Assert(AZStd::holds_alternative<AZStd::monostate>(m_command),
            "Attempting to set FileRequest to 'CreateRequestPathStore', but another task was already assigned.");
        m_command.emplace<RequestPathStoreData>(AZStd::move(path));
        SetOptionalParent(parent);
    }

    void FileRequest::CreateReadRequest(RequestPath path, void* output, u64 outputSize, u64 offset, u64 size,
        AZStd::chrono::system_clock::time_point deadline, IStreamerTypes::Priority priority)
    {
        AZ_Assert(AZStd::holds_alternative<AZStd::monostate>(m_command),
            "Attempting to set FileRequest to 'ReadRequest', but another task was already assigned.");
        m_command.emplace<ReadRequestData>(AZStd::move(path), output, outputSize, offset, size, deadline, priority);
    }

    void FileRequest::CreateReadRequest(RequestPath path, IStreamerTypes::RequestMemoryAllocator* allocator, u64 offset, u64 size,
        AZStd::chrono::system_clock::time_point deadline, IStreamerTypes::Priority priority)
    {
        AZ_Assert(AZStd::holds_alternative<AZStd::monostate>(m_command),
            "Attempting to set FileRequest to 'ReadRequest', but another task was already assigned.");
        m_command.emplace<ReadRequestData>(AZStd::move(path), allocator, offset, size, deadline, priority);
    }

    void FileRequest::CreateRead(FileRequest* parent, void* output, u64 outputSize, const RequestPath& path,
        u64 offset, u64 size, bool sharedRead)
    {
        AZ_Assert(AZStd::holds_alternative<AZStd::monostate>(m_command),
            "Attempting to set FileRequest to 'Read', but another task was already assigned.");
        m_command.emplace<ReadData>(output, outputSize, AZStd::move(path), offset, size, sharedRead);
        SetOptionalParent(parent);
    }

    void FileRequest::CreateCompressedRead(FileRequest* parent, const CompressionInfo& compressionInfo,
        void* output, u64 readOffset, u64 readSize)
    {
        CreateCompressedRead(parent, CompressionInfo(compressionInfo), output, readOffset, readSize);
    }

    void FileRequest::CreateCompressedRead(FileRequest* parent, CompressionInfo&& compressionInfo,
        void* output, u64 readOffset, u64 readSize)
    {
        AZ_Assert(AZStd::holds_alternative<AZStd::monostate>(m_command),
            "Attempting to set FileRequest to 'CompressedRead', but another task was already assigned.");
        m_command.emplace<CompressedReadData>(AZStd::move(compressionInfo), output, readOffset, readSize);
        SetOptionalParent(parent);
    }

    void FileRequest::CreateWait(FileRequest* parent)
    {
        AZ_Assert(AZStd::holds_alternative<AZStd::monostate>(m_command),
            "Attempting to set FileRequest to 'Wait', but another task was already assigned.");
        m_command.emplace<WaitData>();
        SetOptionalParent(parent);
    }

    void FileRequest::CreateFileExistsCheck(const RequestPath& path)
    {
        AZ_Assert(AZStd::holds_alternative<AZStd::monostate>(m_command),
            "Attempting to set FileRequest to 'FileExistsCheck', but another task was already assigned.");
        m_command.emplace<FileExistsCheckData>(path);
    }

    void FileRequest::CreateFileMetaDataRetrieval(const RequestPath& path)
    {
        AZ_Assert(AZStd::holds_alternative<AZStd::monostate>(m_command),
            "Attempting to set FileRequest to 'FileMetaDataRetrieval', but another task was already assigned.");
        m_command.emplace<FileMetaDataRetrievalData>(path);
    }

    void FileRequest::CreateCancel(FileRequestPtr target)
    {
        AZ_Assert(AZStd::holds_alternative<AZStd::monostate>(m_command),
            "Attempting to set FileRequest to 'Cancel', but another task was already assigned.");
        m_command.emplace<CancelData>(AZStd::move(target));
    }

    void FileRequest::CreateReschedule(FileRequestPtr target, AZStd::chrono::system_clock::time_point newDeadline,
        IStreamerTypes::Priority newPriority)
    {
        AZ_Assert(AZStd::holds_alternative<AZStd::monostate>(m_command),
            "Attempting to set FileRequest to 'Reschedule', but another task was already assigned.");
        m_command.emplace<RescheduleData>(AZStd::move(target), newDeadline, newPriority);
    }

    void FileRequest::CreateFlush(RequestPath path)
    {
        AZ_Assert(AZStd::holds_alternative<AZStd::monostate>(m_command),
            "Attempting to set FileRequest to 'Flush', but another task was already assigned.");
        m_command.emplace<FlushData>(AZStd::move(path));
    }

    void FileRequest::CreateFlushAll()
    {
        AZ_Assert(AZStd::holds_alternative<AZStd::monostate>(m_command),
            "Attempting to set FileRequest to 'FlushAll', but another task was already assigned.");
        m_command.emplace<FlushAllData>();
    }

    void FileRequest::CreateDedicatedCacheCreation(RequestPath path, const FileRange& range, FileRequest* parent)
    {
        AZ_Assert(AZStd::holds_alternative<AZStd::monostate>(m_command),
            "Attempting to set FileRequest to 'CreateDedicateCache', but another task was already assigned.");
        m_command.emplace<CreateDedicatedCacheData>(AZStd::move(path), range);
        SetOptionalParent(parent);
    }

    void FileRequest::CreateDedicatedCacheDestruction(RequestPath path, const FileRange& range, FileRequest* parent)
    {
        AZ_Assert(AZStd::holds_alternative<AZStd::monostate>(m_command),
            "Attempting to set FileRequest to 'DestroyDedicateCache', but another task was already assigned.");
        m_command.emplace<DestroyDedicatedCacheData>(AZStd::move(path), range);
        SetOptionalParent(parent);
    }

    void FileRequest::CreateReport(ReportData::ReportType reportType)
    {
        AZ_Assert(AZStd::holds_alternative<AZStd::monostate>(m_command),
            "Attempting to set FileRequest to 'Report', but another task was already assigned.");
        m_command.emplace<ReportData>(reportType);
    }

    void FileRequest::CreateCustom(AZStd::any data, bool failWhenUnhandled, FileRequest* parent)
    {
        AZ_Assert(AZStd::holds_alternative<AZStd::monostate>(m_command),
            "Attempting to set FileRequest to 'Custom', but another task was already assigned.");
        m_command.emplace<CustomData>(AZStd::move(data), failWhenUnhandled);
        SetOptionalParent(parent);
    }

    void FileRequest::SetCompletionCallback(OnCompletionCallback callback)
    {
        m_onCompletion = AZStd::move(callback);
    }

    FileRequest::CommandVariant& FileRequest::GetCommand()
    {
        return m_command;
    }

    const FileRequest::CommandVariant& FileRequest::GetCommand() const
    {
        return m_command;
    }

    IStreamerTypes::RequestStatus FileRequest::GetStatus() const
    {
        return m_status;
    }

    void FileRequest::SetStatus(IStreamerTypes::RequestStatus newStatus)
    {
        IStreamerTypes::RequestStatus currentStatus = m_status;
        switch (newStatus)
        {
        case IStreamerTypes::RequestStatus::Pending:
            [[fallthrough]];
        case IStreamerTypes::RequestStatus::Queued:
            [[fallthrough]];
        case IStreamerTypes::RequestStatus::Processing:
            if (currentStatus == IStreamerTypes::RequestStatus::Failed ||
                currentStatus == IStreamerTypes::RequestStatus::Canceled ||
                currentStatus == IStreamerTypes::RequestStatus::Completed)
            {
                return;
            }
            break;
        case IStreamerTypes::RequestStatus::Completed:
            if (currentStatus == IStreamerTypes::RequestStatus::Failed || currentStatus == IStreamerTypes::RequestStatus::Canceled)
            {
                return;
            }
            break;
        case IStreamerTypes::RequestStatus::Canceled:
            if (currentStatus == IStreamerTypes::RequestStatus::Failed || currentStatus == IStreamerTypes::RequestStatus::Completed)
            {
                return;
            }
            break;
        case IStreamerTypes::RequestStatus::Failed:
            [[fallthrough]];
        default:
            break;
        }
        m_status = newStatus;
    }

    FileRequest* FileRequest::GetParent()
    {
        return m_parent;
    }

    const FileRequest* FileRequest::GetParent() const
    {
        return m_parent;
    }

    size_t FileRequest::GetNumDependencies() const
    {
        return m_dependencies;
    }

    bool FileRequest::FailsWhenUnhandled() const
    {
        return AZStd::visit([](auto&& args)
        {
            using Command = AZStd::decay_t<decltype(args)>;
            if constexpr (AZStd::is_same_v<Command, AZStd::monostate>)
            {
                AZ_Assert(false,
                    "Request does not contain a valid command. It may have been reset already or was never assigned a command.");
                return true;
            }
            else if constexpr (AZStd::is_same_v<Command, FileRequest::CustomData>)
            {
                return args.m_failWhenUnhandled;
            }
            else
            {
                return Command::s_failWhenUnhandled;
            }
        }, m_command);
    }

    void FileRequest::Reset()
    {
        m_command = AZStd::monostate{};
        m_onCompletion = &OnCompletionPlaceholder;
        m_estimatedCompletion = AZStd::chrono::system_clock::time_point();
        m_parent = nullptr;
        m_status = IStreamerTypes::RequestStatus::Pending;
        m_dependencies = 0;
    }

    void FileRequest::SetOptionalParent(FileRequest* parent)
    {
        if (parent)
        {
            m_parent = parent;
            AZ_Assert(parent->m_dependencies < std::numeric_limits<decltype(parent->m_dependencies)>::max(),
                "A file request dependency was added, but the parent can't have any more dependencies.");
            ++parent->m_dependencies;
        }
    }

    bool FileRequest::WorksOn(FileRequestPtr& request) const
    {
        const FileRequest* current = this;
        while (current)
        {
            auto* link = AZStd::get_if<ExternalRequestData>(&current->m_command);
            if (!link)
            {
                current = current->m_parent;
            }
            else
            {
                return link->m_request == request;
            }
        }
        return false;
    }

    size_t FileRequest::GetPendingId() const
    {
        return m_pendingId;
    }

    void FileRequest::SetEstimatedCompletion(AZStd::chrono::system_clock::time_point time)
    {
        FileRequest* current = this;
        do
        {
            current->m_estimatedCompletion = time;
            current = current->m_parent;
        } while (current);
    }

    AZStd::chrono::system_clock::time_point FileRequest::GetEstimatedCompletion() const
    {
        return m_estimatedCompletion;
    }

    //
    // ExternalFileRequest
    //

    ExternalFileRequest::ExternalFileRequest(StreamerContext* owner)
        : m_request(FileRequest::Usage::External)
        , m_owner(owner)
    {
    }

    void ExternalFileRequest::add_ref()
    {
        m_refCount++;
    }

    void ExternalFileRequest::release()
    {
        if (--m_refCount == 0)
        {
            AZ_Assert(m_owner, "No owning context set for the file request.");
            m_owner->RecycleRequest(this);
        }
    }

    bool operator==(const FileRequestHandle& lhs, const FileRequestPtr& rhs)
    {
        return lhs.m_request == &rhs->m_request;
    }

    bool operator==(const FileRequestPtr& lhs, const FileRequestHandle& rhs)
    {
        return rhs == lhs;
    }

    bool operator!=(const FileRequestHandle& lhs, const FileRequestPtr& rhs)
    {
        return !(lhs == rhs);
    }

    bool operator!=(const FileRequestPtr& lhs, const FileRequestHandle& rhs)
    {
        return !(rhs == lhs);
    }
} // namespace AZ::IO
