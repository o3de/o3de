/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomPath.h>
#include <AzCore/Name/NameDictionary.h>
#include <Tests/DOM/DomFixtures.h>

namespace AZ::Dom::Tests
{
    class DomPathTests : public DomTestFixture
    {
    };

    TEST_F(DomPathTests, EmptyPath_IsEmpty)
    {
        Path path("");
        EXPECT_EQ(path.GetEntries().size(), 0);
    }

    TEST_F(DomPathTests, EmptyPath_IsEqualToDefault)
    {
        EXPECT_EQ(Path(""), Path());
    }

    TEST_F(DomPathTests, Index_IsNumeric)
    {
        EXPECT_EQ(Path("/0")[0], 0);
        EXPECT_EQ(Path("/20")[0], 20);
        EXPECT_EQ(Path("/9999")[0], 9999);
        EXPECT_EQ(Path("/0/4/5/1")[3], 1);
    }

    TEST_F(DomPathTests, Index_ConvertsToString)
    {
        Path p;
        EXPECT_EQ(p.ToString(), "");

        p.Push(0);
        EXPECT_EQ(p.ToString(), "/0");

        p.Push(1);
        EXPECT_EQ(p.ToString(), "/0/1");

        p.Push(10);
        EXPECT_EQ(p.ToString(), "/0/1/10");

        p.Push(9999);
        EXPECT_EQ(p.ToString(), "/0/1/10/9999");

        p.Pop();
        EXPECT_EQ(p.ToString(), "/0/1/10");
    }

    TEST_F(DomPathTests, Key_IsString)
    {
        EXPECT_EQ(Path("/foo")[0], "foo");
        EXPECT_EQ(Path("/bar")[0], "bar");
        EXPECT_EQ(Path("/foo/bar/baz12345")[0], "foo");
        EXPECT_EQ(Path("/foo/bar/baz12345")[2], "baz12345");
        EXPECT_EQ(Path("//foo")[0], "");
    }

    TEST_F(DomPathTests, Key_ConvertsToString)
    {
        Path p;

        p.Push("foo");
        EXPECT_EQ(p.ToString(), "/foo");

        p.Push("bar");
        EXPECT_EQ(p.ToString(), "/foo/bar");

        p.Push("another_key");
        EXPECT_EQ(p.ToString(), "/foo/bar/another_key");

        p.Pop();
        EXPECT_EQ(p.ToString(), "/foo/bar");
    }

    TEST_F(DomPathTests, Key_ConvertsFromEscaped)
    {
        EXPECT_EQ(Path("/foo~0")[0], "foo~");
        EXPECT_EQ(Path("/foo~0bar~0~0")[0], "foo~bar~~");
        EXPECT_EQ(Path("/foo~1bar/baz")[0], "foo/bar");
        EXPECT_EQ(Path("/~1foo~1")[0], "/foo/");
    }

    TEST_F(DomPathTests, Key_ConvertsToEscaped)
    {
        Path p;

        p.Push("with~tilde");
        EXPECT_EQ(p.ToString(), "/with~0tilde");

        p.Push("with/slash");
        EXPECT_EQ(p.ToString(), "/with~0tilde/with~1slash");

        p.Clear();
        p.Push("/~with/mixed/characters~");
        EXPECT_EQ(p.ToString(), "/~1~0with~1mixed~1characters~0");
    }

    TEST_F(DomPathTests, MixedPath_Resolves)
    {
        EXPECT_EQ(Path("/foo/0")[0], "foo");
        EXPECT_EQ(Path("/foo/0")[1], 0);
        EXPECT_EQ(Path("/42/foo/bar/0")[0], 42);
        EXPECT_EQ(Path("/42/foo/bar/0")[1], "foo");
        EXPECT_EQ(Path("/42/foo/bar/0")[2], "bar");
        EXPECT_EQ(Path("/42/foo/bar/0")[3], 0);
    }

    TEST_F(DomPathTests, MixedPath_ConvertsToString)
    {
        Path p;

        p.Push("foo");
        EXPECT_EQ(p.ToString(), "/foo");

        p.Push(0);
        EXPECT_EQ(p.ToString(), "/foo/0");

        p.Push("another_key");
        EXPECT_EQ(p.ToString(), "/foo/0/another_key");

        p.Push(100);
        EXPECT_EQ(p.ToString(), "/foo/0/another_key/100");

        p.Pop();
        EXPECT_EQ(p.ToString(), "/foo/0/another_key");
    }

    TEST_F(DomPathTests, OperatorOverloads_Append)
    {
        EXPECT_EQ(Path("/foo/bar"), Path("/foo") / Path("/bar"));
        EXPECT_EQ(Path("/foo"), Path("/foo") / Path());
        EXPECT_EQ(Path("/foo/1/bar/0"), Path("/foo/1") / Path("/bar/0"));
        EXPECT_EQ(Path("/foo") / 0, Path("/foo/0"));
        EXPECT_EQ(Path() / "foo" / "bar", Path("/foo/bar"));
        EXPECT_EQ(Path("/foo") / "bar" / "baz", Path("/foo/bar/baz"));

        Path p("/foo/bar");
        p /= "baz";
        EXPECT_EQ(p, Path("/foo/bar/baz"));
        p /= Path("0/1");
        EXPECT_EQ(p, Path("/foo/bar/baz/0/1"));
    }

    TEST_F(DomPathTests, EndOfArray_FromString)
    {
        EXPECT_FALSE(Path("/foo/-")[0].IsEndOfArray());
        EXPECT_TRUE(Path("/foo/-")[1].IsEndOfArray());
        EXPECT_TRUE(Path("/foo/-/-")[2].IsEndOfArray());
    }

    TEST_F(DomPathTests, EndOfArray_ToString)
    {
        EXPECT_EQ(Path("/-").ToString(), "/-");
        EXPECT_EQ(Path("/0/-").ToString(), "/0/-");
    }

    TEST_F(DomPathTests, MixedPath_AppendToString)
    {
        Path p("/foo/0");
        AZStd::string s;

        p.AppendToString(s);
        EXPECT_EQ(s, "/foo/0");
        p.AppendToString(s);
        EXPECT_EQ(s, "/foo/0/foo/0");
    }
} // namespace AZ::Dom::Tests
