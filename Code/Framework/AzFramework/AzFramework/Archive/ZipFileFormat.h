/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/base.h>
#include <AzCore/std/algorithm.h>

namespace AZ::IO::ZipFile
{
    using HeaderType = uint32_t;

    // General-purpose bit field flags
    enum
    {
        GPF_ENCRYPTED = 1, // If set, indicates that the file is encrypted.
        GPF_DATA_DESCRIPTOR = 1 << 3, // if set, the CRC32 and sizes aren't set in the file header, but only in the data descriptor following compressed data
        GPF_RESERVED_8_ENHANCED_DEFLATING = 1 << 4, // Reserved for use with method 8, for enhanced deflating.
        GPF_COMPRESSED_PATCHED = 1 << 5, // the file is compressed patched data
    };

    enum
    {
        BLOCK_CIPHER_NUM_KEYS = 16
    };
    enum
    {
        BLOCK_CIPHER_KEY_LENGTH = 16
    };
    enum
    {
        RSA_KEY_MESSAGE_LENGTH = 128    //The modulus of our private/public key pair for signing, verification, encryption and decryption
    };

    // compression methods
    enum
    {
        METHOD_STORE = 0, // The file is stored (no compression)
        METHOD_SHRINK = 1, // The file is Shrunk
        METHOD_REDUCE_1 = 2, // The file is Reduced with compression factor 1
        METHOD_REDUCE_2 = 3, // The file is Reduced with compression factor 2
        METHOD_REDUCE_3 = 4, // The file is Reduced with compression factor 3
        METHOD_REDUCE_4 = 5, // The file is Reduced with compression factor 4
        METHOD_IMPLODE = 6, // The file is Imploded
        METHOD_TOKENIZE = 7, // Reserved for Tokenizing compression algorithm
        METHOD_DEFLATE = 8, // The file is Deflated
        METHOD_DEFLATE64 = 9, // Enhanced Deflating using Deflate64(tm)
        METHOD_IMPLODE_PKWARE = 10, // PKWARE Date Compression Library Imploding
        METHOD_DEFLATE_AND_ENCRYPT = 11, // Deflate + Custom encryption (TEA)
        METHOD_DEFLATE_AND_STREAMCIPHER = 12, // Deflate + stream cipher encryption on a per file basis
        METHOD_STORE_AND_STREAMCIPHER_KEYTABLE = 13, // Store + Timur's encryption technique on a per file basis
        METHOD_DEFLATE_AND_STREAMCIPHER_KEYTABLE = 14, // Deflate + Timur's encryption technique on a per file basis
    };


    // end of Central Directory Record
    // followed by the .zip file comment (variable size, can be empty, obtained from nCommentLength)
#pragma pack(push, 1)
    struct CDREnd
    {
        inline static constexpr uint32_t SIGNATURE = 0x06054b50;

        uint32_t lSignature{};       // end of central dir signature    4 bytes  (0x06054b50)
        uint16_t nDisk{};            // number of this disk             2 bytes
        uint16_t nCDRStartDisk{};    // number of the disk with the start of the central directory  2 bytes
        uint16_t numEntriesOnDisk{}; // total number of entries in the central directory on this disk  2 bytes
        uint16_t numEntriesTotal{};  // total number of entries in the central directory           2 bytes
        uint32_t lCDRSize{};          // size of the central directory   4 bytes
        uint32_t lCDROffset{};        // offset of start of central directory with respect to the starting disk number        4 bytes
        uint16_t nCommentLength{};   // .ZIP file comment length        2 bytes

        // .ZIP file comment (variable size, can be empty) follows
    };
#pragma pack(pop)

    // encryption settings for zip header - stored in m_headerExtended struct
    enum EHeaderEncryptionType : int32_t
    {
        HEADERS_NOT_ENCRYPTED = 0,
        HEADERS_ENCRYPTED_STREAMCIPHER = 1,
        HEADERS_ENCRYPTED_TEA = 2,                        //TEA = Tiny Encryption Algorithm
        HEADERS_ENCRYPTED_STREAMCIPHER_KEYTABLE = 3,          //Timur's technique. Encrypt each file and the CDR with one of 16 stream cipher keys. Encrypt the table of keys with an RSA key.
    };
    // Signature settings for zip header
    enum EHeaderSignatureType : int32_t
    {
        HEADERS_NOT_SIGNED = 0,
        HEADERS_CDR_SIGNED = 1    //Includes an RSA signature based on the hash of the archive's CDR. Verified in a console compatible way.
    };

    //Header for HEADERS_ENCRYPTED_CRYCUSTOM technique. Paired with a CrySignedCDRHeader to allow for signing as well as encryption.
    //i.e. the comment section for a file that uses this technique needs the following in order:
    //CryCustomExtendedHeader, CrySignedCDRHeader, CryCustomEncryptionHeader
#pragma pack(push, 1)
    struct CryCustomEncryptionHeader
    {
        uint32_t nHeaderSize{}; // Size of the extended header.
        uint8_t CDR_IV[RSA_KEY_MESSAGE_LENGTH];                                                           //Initial Vector is actually BLOCK_CIPHER_KEY_LENGTH bytes in length, but is encrypted as a RSA_KEY_MESSAGE_LENGTH byte message.
        uint8_t keys_table[BLOCK_CIPHER_NUM_KEYS][RSA_KEY_MESSAGE_LENGTH];    //As above, actually BLOCK_CIPHER_KEY_LENGTH but encrypted.
    };
#pragma pack(pop)

    //Header for HEADERS_SIGNED_CDR technique implemented on consoles. The comment section needs to contain the following in order:
    //CryCustomExtendedHeader, CrySignedCDRHeader
#pragma pack(push, 1)
    struct CrySignedCDRHeader
    {
        uint32_t nHeaderSize{}; // Size of the extended header.
        uint8_t CDR_signed[RSA_KEY_MESSAGE_LENGTH];
    };
#pragma pack(pop)

    //Stores type of encryption and signing
#pragma pack(push, 1)
    struct CryCustomExtendedHeader
    {
        uint32_t nHeaderSize{}; // Size of the extended header.
        uint16_t nEncryption{}; // Matches one of EHeaderEncryptionType: 0 = No encryption/extension
        uint16_t nSigning{}; // Matches one of EHeaderSignatureType: 0 = No signing
    };
#pragma pack(pop)

    // This descriptor exists only if bit 3 of the general
    // purpose bit flag is set (see below).  It is byte aligned
    // and immediately follows the last byte of compressed data.
    // This descriptor is used only when it was not possible to
    // seek in the output .ZIP file, e.g., when the output .ZIP file
    // was standard output or a non seekable device.  For Zip64 format
    // archives, the compressed and uncompressed sizes are 8 bytes each.
#pragma pack(push, 1)
    struct DataDescriptor
    {
        uint32_t lCRC32{};             // crc-32                          4 bytes
        uint32_t lSizeCompressed{};    // compressed size                 4 bytes
        uint32_t lSizeUncompressed{};  // uncompressed size               4 bytes

        bool operator==(const DataDescriptor& d) const
        {
            return lCRC32 == d.lCRC32 && lSizeCompressed == d.lSizeCompressed && lSizeUncompressed == d.lSizeUncompressed;
        }
        bool operator!=(const DataDescriptor& d) const
        {
            return lCRC32 != d.lCRC32 || lSizeCompressed != d.lSizeCompressed || lSizeUncompressed != d.lSizeUncompressed;
        }
    };
#pragma pack(pop)

    // the File Header as it appears in the CDR
    // followed by:
    //    file name (variable size)
    //    extra field (variable size)
    //    file comment (variable size)
#pragma pack(push, 1)
    struct CDRFileHeader
    {
        inline static constexpr uint32_t SIGNATURE = 0x02014b50;

        uint32_t lSignature{};         // central file header signature   4 bytes  (0x02014b50)
        uint16_t nVersionMadeBy{};     // version made by                 2 bytes
        uint16_t nVersionNeeded{};     // version needed to extract       2 bytes
        uint16_t nFlags{};             // general purpose bit flag        2 bytes
        uint16_t nMethod{};            // compression method              2 bytes
        uint16_t nLastModTime{};       // last mod file time              2 bytes
        uint16_t nLastModDate{};       // last mod file date              2 bytes
        DataDescriptor desc{};
        uint16_t nFileNameLength{};    // file name length                2 bytes
        uint16_t nExtraFieldLength{};  // extra field length              2 bytes
        uint16_t nFileCommentLength{}; // file comment length             2 bytes
        uint16_t nDiskNumberStart{};   // disk number start               2 bytes
        uint16_t nAttrInternal{};      // internal file attributes        2 bytes
        uint32_t lAttrExternal{};      // external file attributes        4 bytes

        // This is the offset from the start of the first disk on
        // which this file appears, to where the local header should
        // be found.  If an archive is in zip64 format and the value
        // in this field is 0xFFFFFFFF, the size will be in the
        // corresponding 8 byte zip64 extended information extra field.
        enum
        {
            ZIP64_LOCAL_HEADER_OFFSET = 0xFFFFFFFF
        };
        uint32_t lLocalHeaderOffset{}; // relative offset of local header 4 bytes
    };
#pragma pack(pop)

    // this is the local file header that appears before the compressed data
    // followed by:
    //    file name (variable size)
    //    extra field (variable size)
#pragma pack(push, 1)
    struct LocalFileHeader
    {
        inline static constexpr uint32_t SIGNATURE = 0x04034b50;
        uint32_t lSignature{};        // local file header signature     4 bytes  (0x04034b50)
        uint16_t nVersionNeeded{};    // version needed to extract       2 bytes
        uint16_t nFlags{};            // general purpose bit flag        2 bytes
        uint16_t nMethod{};           // compression method              2 bytes
        uint16_t nLastModTime{};      // last mod file time              2 bytes
        uint16_t nLastModDate{};      // last mod file date              2 bytes
        DataDescriptor desc{};
        uint16_t nFileNameLength{};   // file name length                2 bytes
        uint16_t nExtraFieldLength{}; // extra field length              2 bytes
    };
#pragma pack(pop)

    // compression methods
    enum EExtraHeaderID : uint32_t
    {
        EXTRA_ZIP64 = 0x0001, //  ZIP64 extended information extra field
        EXTRA_NTFS = 0x000a, //  NTFS
    };

    //////////////////////////////////////////////////////////////////////////
    // header1+data1 + header2+data2 . . .
    // Each header should consist of:
    //      Header ID - 2 bytes
    //    Data Size - 2 bytes
    struct ExtraFieldHeader
    {
        uint16_t headerID{};
        uint16_t dataSize{};
    };
    struct ExtraNTFSHeader
    {
        uint32_t reserved{};  // 4 bytes.
        uint16_t attrTag{};  // 2 bytes.
        uint16_t attrSize{}; // 2 bytes.
    };
}

namespace AZStd
{
    // Specialize AZStd::endian_swap function for ZipFile structures
    inline void endian_swap(AZ::IO::ZipFile::CDREnd& data)
    {
        AZStd::endian_swap(data.lSignature);
        AZStd::endian_swap(data.nDisk);
        AZStd::endian_swap(data.nCDRStartDisk);
        AZStd::endian_swap(data.numEntriesOnDisk);
        AZStd::endian_swap(data.numEntriesTotal);
        AZStd::endian_swap(data.lCDRSize);
        AZStd::endian_swap(data.lCDROffset);
        AZStd::endian_swap(data.nCommentLength);
    }
    inline void endian_swap(AZ::IO::ZipFile::DataDescriptor& data)
    {
        AZStd::endian_swap(data.lCRC32);
        AZStd::endian_swap(data.lSizeCompressed);
        AZStd::endian_swap(data.lSizeUncompressed);
    }
    inline void endian_swap(AZ::IO::ZipFile::CDRFileHeader& data)
    {
        AZStd::endian_swap(data.lSignature);
        AZStd::endian_swap(data.nVersionMadeBy);
        AZStd::endian_swap(data.nVersionNeeded);
        AZStd::endian_swap(data.nFlags);
        AZStd::endian_swap(data.nMethod);
        AZStd::endian_swap(data.nLastModTime);
        AZStd::endian_swap(data.nLastModDate);
        AZStd::endian_swap(data.desc);
        AZStd::endian_swap(data.nFileNameLength);
        AZStd::endian_swap(data.nExtraFieldLength);
        AZStd::endian_swap(data.nFileCommentLength);
        AZStd::endian_swap(data.nDiskNumberStart);
        AZStd::endian_swap(data.nAttrInternal);
        AZStd::endian_swap(data.lAttrExternal);
    }
    inline void endian_swap(AZ::IO::ZipFile::LocalFileHeader& data)
    {
        AZStd::endian_swap(data.lSignature);
        AZStd::endian_swap(data.nVersionNeeded);
        AZStd::endian_swap(data.nFlags);
        AZStd::endian_swap(data.nMethod);
        AZStd::endian_swap(data.nLastModTime);
        AZStd::endian_swap(data.nLastModDate);
        AZStd::endian_swap(data.desc);
        AZStd::endian_swap(data.nFileNameLength);
        AZStd::endian_swap(data.nExtraFieldLength);
    }
}
