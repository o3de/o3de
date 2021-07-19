/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

// AzTest
#include <AzTest/AzTest.h>

// Qt
#include <QBuffer>

static QString CreateTestString(int size)
{
    QString testString;
    testString.resize(size);

    // generate a string with some kind of pattern, so that
    // decoding is verified against something semi-realistic
    QString pattern = "TestPattern";
    for (int i = 0; i < size; i++)
    {
        testString[i] = pattern[i % pattern.length()];
    }

    return testString;
}

TEST(CArchive, 1ByteLengthString)
{
    QString outString = CreateTestString(20); // use a test size less than 2^8, so it takes only 1 byte for the length

    QBuffer memBlock;
    memBlock.open(QIODevice::ReadWrite);
    CArchive writeArchive(&memBlock, CArchive::store);
    writeArchive << outString;

    QByteArray dataBuffer = memBlock.buffer();

    // buffer size after write is the 1 byte for the string length followed by the string
    EXPECT_EQ(memBlock.pos(), (1 + outString.length()));

    const char* data = dataBuffer.constData();
    EXPECT_EQ(data[0], outString.length());

    for (int i = 0; i < outString.length(); i++)
    {
        EXPECT_EQ(data[1 + i], outString[i]);
    }

    // now confirm that we can read it back out
    memBlock.reset();
    CArchive readArchive(&memBlock, CArchive::load);
    QString inString;
    readArchive >> inString;

    EXPECT_EQ(inString, outString);
}

TEST(CArchive, test2ByteLengthString)
{
    int testSize = 0xff + 1; // use a test size greater than 2^8, so it takes more than 1 byte for the length
    QString outString = CreateTestString(testSize);

    QBuffer memBlock;
    memBlock.open(QIODevice::ReadWrite);
    CArchive writeArchive(&memBlock, CArchive::store);
    writeArchive << outString;

    QByteArray dataBuffer = memBlock.buffer();

    // buffer size after write is the 1 marker byte (0xff), 2 bytes for the string length followed by the string
    EXPECT_EQ(memBlock.pos(), (1 + 2 + testSize));

    const char* data = dataBuffer.constData();
    EXPECT_EQ(data[0], '\xff');
    short shortLength = 0;
    memcpy(&shortLength, data + 1, 2);
    EXPECT_EQ(shortLength, testSize);

    for (int i = 0; i < testSize; i++)
    {
        EXPECT_EQ(data[3 + i], outString[i]);
    }

    // now confirm that we can read it back out
    memBlock.reset();
    CArchive readArchive(&memBlock, CArchive::load);
    QString inString;
    readArchive >> inString;

    EXPECT_EQ(inString, outString);
}

TEST(CArchive, test4ByteLengthString)
{
    int testSize = 0xffff + 1; // use a test size greater than 2^16, so it takes more than 2 bytes for the length
    QString outString = CreateTestString(testSize);

    QBuffer memBlock;
    memBlock.open(QIODevice::ReadWrite);
    CArchive writeArchive(&memBlock, CArchive::store);
    writeArchive << outString;

    QByteArray dataBuffer = memBlock.buffer();

    // buffer size after write is the 1 marker byte (0xff), the 2 byte marker (0xffff), 4 bytes for the string length followed by the string
    EXPECT_EQ(memBlock.pos(), (1 + 2 + 4 + testSize));

    const char* data = dataBuffer.constData();
    EXPECT_EQ(data[0], '\xff');
    EXPECT_EQ(data[1], '\xff');
    EXPECT_EQ(data[2], '\xff');
    uint32 doubleWordLength = 0;
    memcpy(&doubleWordLength, data + 3, 4);
    EXPECT_EQ(doubleWordLength, testSize);

    for (int i = 0; i < testSize; i++)
    {
        EXPECT_EQ(data[7 + i], outString[i]);
    }

    // now confirm that we can read it back out
    memBlock.reset();
    CArchive readArchive(&memBlock, CArchive::load);
    QString inString;
    readArchive >> inString;

    EXPECT_EQ(inString, outString);
}

TEST(CArchive, testWindowsWideCharacterString1ByteLength)
{
    // NOTE: CArchive only writes out Utf-8 now, so we can't actually
    // use that class to encode to Windows wide character strings.
    // That is why this test (unlike the non-Windows wide character tests)
    // is manually creating a stream and only confirming that CArchive can decode it.

    QString outString = CreateTestString(20); // use a test size smaller than 2^8, so that we only need one byte to store the length

    // we have to write the 0xff marker, then the 2 byte marker to indicate it's wide characters, 
    // then the actual length (1 byte), then the string, with windows wide characters (2 bytes per)
    QByteArray testBuffer;
    testBuffer.resize(1 + 2 + 1 + (outString.length() * 2));
    testBuffer[0] = '\xff';
    testBuffer[1] = '\xfe';
    testBuffer[2] = '\xff';
    testBuffer[3] = aznumeric_cast<char>(outString.length());

    memcpy(testBuffer.data() + 4, outString.utf16(), outString.length() * 2);

    // now confirm that we can read it back out
    QBuffer memBlock(&testBuffer);
    memBlock.open(QIODevice::ReadOnly);
    CArchive readArchive(&memBlock, CArchive::load);
    QString inString;
    readArchive >> inString;

    EXPECT_EQ(inString, outString);
}

TEST(CArchive, testWindowsWideCharacterString2ByteLength)
{
    // NOTE: CArchive only writes out Utf-8 now, so we can't actually
    // use that class to encode to Windows wide character strings.
    // That is why this test (unlike the non-Windows wide character tests)
    // is manually creating a stream and only confirming that CArchive can decode it.

    int testSize = 0xff + 1; // use a test size greater than 2^8, so that we need two bytes to store the length
    QString outString = CreateTestString(testSize);

    // we have to write the 0xff marker, then the 2 byte marker to indicate it's wide characters, 
    // then the marker to indicate it's more than a 1 byte length, then the actual length (2 byte),
    // then the string, with windows wide characters (2 bytes per)
    QByteArray testBuffer;
    testBuffer.resize(1 + 2 + 1 + 2 + (testSize * 2));
    testBuffer[0] = '\xff';
    testBuffer[1] = '\xfe';
    testBuffer[2] = '\xff';
    testBuffer[3] = '\xff';

    memcpy(testBuffer.data() + 4, &testSize, 2);

    memcpy(testBuffer.data() + 6, outString.utf16(), testSize * 2);

    // now confirm that we can read it back out 
    QBuffer memBlock(&testBuffer);
    memBlock.open(QIODevice::ReadOnly);
    CArchive readArchive(&memBlock, CArchive::load);
    QString inString;
    readArchive >> inString;

    EXPECT_EQ(inString, outString);
}
