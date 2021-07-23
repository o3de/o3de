/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef ASSETBUILDER_BYTEARRAYSTREAM
#define ASSETBUILDER_BYTEARRAYSTREAM

#include <AzCore/IO/GenericStreams.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>

#include <QByteArray>

namespace AssetProcessor
{
    //! Wrap a QByteArray (which has an int-interface) in a GenericStream.
    class ByteArrayStream
        : public AZ::IO::GenericStream
    {
    public:
        ByteArrayStream();
        ByteArrayStream(QByteArray* other); // attach to external
        ByteArrayStream(const char* data, unsigned int length); // const attach to read-only buffer

        bool IsOpen() const override
        {
            return true;
        }
        bool CanSeek() const override
        {
            return true;
        }

        virtual bool        CanRead() const override { return true; }
        virtual bool        CanWrite() const override { return true; }

        void Seek(AZ::IO::OffsetType bytes, SeekMode mode) override;
        AZ::IO::SizeType Read(AZ::IO::SizeType bytes, void* oBuffer) override;
        AZ::IO::SizeType Write(AZ::IO::SizeType bytes, const void* iBuffer) override;
        AZ::IO::SizeType WriteFromStream(AZ::IO::SizeType bytes, AZ::IO::GenericStream *inputStream) override;
        AZ::IO::SizeType GetCurPos() const override;
        AZ::IO::SizeType GetLength() const override;

        QByteArray GetArray() const; // bytearrays are copy-on-write so retrieving it is akin to retreiving a refcounted object, its cheap to 'copy'

        void Reserve(int amount); // for performance.

    private:
        AZ::IO::SizeType PrepareForWrite(AZ::IO::SizeType bytes);

        QByteArray* m_activeArray;
        QByteArray m_ownArray; // used when not constructed around an attached array
        bool m_usingOwnArray = true; // if false, its been attached
        int m_currentPos = 0; // the byte array underlying has only ints :(
        bool m_readOnly = false;
    };

    // Pack any serializable type into a QByteArray
    // note that this is not a specialization of the AZFramework version of this function
    // because C++ does not support partial specialization of function templates, only classes.
    template <class Message>
    bool PackMessage(const Message& message, QByteArray& buffer)
    {
        ByteArrayStream byteStream(&buffer);
        return AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_BINARY, &message, message.RTTI_GetType());
    }

    // Unpack any serializable type from a QByteArray
    // note that this is not a specialization of the AZFramework version of this function
    // because C++ does not support partial specialization of function templates, only classes.
    template <class Message>
    bool UnpackMessage(const QByteArray& buffer, Message& message)
    {
        ByteArrayStream byteStream(buffer.constData(), buffer.size());
        // we expect network messages to be pristine - so if there's any error, don't allow it! 
        // also do not allow it to load assets just becuase they're in fields
        AZ::ObjectStream::FilterDescriptor filterToUse(&AZ::Data::AssetFilterNoAssetLoading, AZ::ObjectStream::FILTERFLAG_STRICT);
        return AZ::Utils::LoadObjectFromStreamInPlace(byteStream, message, nullptr, filterToUse);
    }
}



#endif // ASSETBUILDER_BYTEARRAYSTREAM
