/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(SHINE_INTERNAL_UNIT_TEST)

#include <IConsole.h>

namespace
{
    void AnchorTagTests()
    {
        const Shine::StringType rawCharData("this is a test!");

        ///////////////////////////////////////////////////////////////
        // Valid markup tests

        // Wrap just "test" in an anchor
        {
            Shine::StringType source = "this is a <a action=\"action\" data=\"data\">test</a>!";
            Shine::StringType expectedMarkup = "<root><ch value=\"this is a \" /><a action=\"action\" data=\"data\"><ch value=\"test\" /></a><ch value=\"!\" /></root>";

            Shine::StringType markupTarget;
            InsertMarkup(source, markupTarget);
            AZ_Assert(expectedMarkup == markupTarget, "Test failed");

            Shine::StringType target;
            TextMarkup::CopyCharData(source, target);
            AZ_Assert(rawCharData == target, "Test failed");
        }

        // Wrap whole string in an anchor
        {
            Shine::StringType source = "<a action=\"action\" data=\"data\">this is a test!</a>";
            Shine::StringType expectedMarkup = "<root><a action=\"action\" data=\"data\"><ch value=\"this is a test!\" /></a></root>";

            Shine::StringType markupTarget;
            InsertMarkup(source, markupTarget);
            AZ_Assert(expectedMarkup == markupTarget, "Test failed");

            Shine::StringType target;
            TextMarkup::CopyCharData(source, target);
            AZ_Assert(rawCharData == target, "Test failed");
        }

        // Two anchor tags: "this" and "test"
        {
            Shine::StringType source = "<a action=\"action\" data=\"data\">this</a> is a <a action=\"action\" data=\"data\">test</a>!";
            Shine::StringType expectedMarkup = "<root><a action=\"action\" data=\"data\"><ch value=\"this\" /></a><ch value=\" is a \" /><a action=\"action\" data=\"data\"><ch value=\"test\" /></a><ch value=\"!\" /></root>";

            Shine::StringType markupTarget;
            InsertMarkup(source, markupTarget);
            AZ_Assert(expectedMarkup == markupTarget, "Test failed");

            Shine::StringType target;
            TextMarkup::CopyCharData(source, target);
            AZ_Assert(rawCharData == target, "Test failed");
        }

        // Wrap just "test" in an anchor; only has 'action' attribute
        {
            Shine::StringType source = "this is a <a action=\"action\">test</a>!";
            Shine::StringType expectedMarkup = "<root><ch value=\"this is a \" /><a action=\"action\"><ch value=\"test\" /></a><ch value=\"!\" /></root>";

            Shine::StringType markupTarget;
            InsertMarkup(source, markupTarget);
            AZ_Assert(expectedMarkup == markupTarget, "Test failed");

            Shine::StringType target;
            TextMarkup::CopyCharData(source, target);
            AZ_Assert(rawCharData == target, "Test failed");
        }

        // Wrap just "test" in an anchor; only has 'data' attribute
        {
            Shine::StringType source = "this is a <a data=\"data\">test</a>!";
            Shine::StringType expectedMarkup = "<root><ch value=\"this is a \" /><a data=\"data\"><ch value=\"test\" /></a><ch value=\"!\" /></root>";

            Shine::StringType markupTarget;
            InsertMarkup(source, markupTarget);
            AZ_Assert(expectedMarkup == markupTarget, "Test failed");

            Shine::StringType target;
            TextMarkup::CopyCharData(source, target);
            AZ_Assert(rawCharData == target, "Test failed");
        }

        ///////////////////////////////////////////////////////////////
        // Bad xml tests

#if 0
        // Nested anchor tags
        {
            Shine::StringType source = "<a action=\"action\" data=\"data\"><a action=\"action\" data=\"data\">this</a></a> is a test!";
            Shine::StringType target;
            TextMarkup::CopyCharData(source, target);
            AZ_Assert(source == target, "Test failed");
        }
#endif

        // Anchor tags with no attributes
        {
            Shine::StringType source = "this is a <a>test</a>!";
            Shine::StringType target;
            TextMarkup::CopyCharData(source, target);
            AZ_Assert(source == target, "Test failed");
        }

        // Anchor tags with invalid attributes
        {
            Shine::StringType source = "this is a <a bad=\"bad\">test</a>!";
            Shine::StringType target;
            TextMarkup::CopyCharData(source, target);
            AZ_Assert(source == target, "Test failed");
        }

        // Anchor tags with valid and invalid attributes
        {
            Shine::StringType source = "this is a <a action=\"action\" bad=\"bad\">test</a>!";
            Shine::StringType target;
            TextMarkup::CopyCharData(source, target);
            AZ_Assert(source == target, "Test failed");
        }

        {
            Shine::StringType source = "this is a <a data=\"data\" bad=\"bad\">test</a>!";
            Shine::StringType target;
            TextMarkup::CopyCharData(source, target);
            AZ_Assert(source == target, "Test failed");
        }

        {
            Shine::StringType source = "this is a <a action=\"action\" data=\"data\" bad=\"bad\">test</a>!";
            Shine::StringType target;
            TextMarkup::CopyCharData(source, target);
            AZ_Assert(source == target, "Test failed");
        }
    }
}

void TextMarkup::UnitTest(IConsoleCmdArgs* /* cmdArgs*/)
{
    ///////////////////////////////////////////////////////////////
    // Expected inputs: general
    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source = "this <i>is</i> a <b>test</b>!";
        Shine::StringType expectedMarkup = "<root><ch value=\"this \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source = "<b>this</b> <i>is</i> a <b>test</b>!";
        Shine::StringType expectedMarkup = "<root><b><ch value=\"this\" /></b><ch value=\" \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    ///////////////////////////////////////////////////////////////
    // Expected inputs: <font> tag
    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source = "<font face=\"times\">this</font> <i>is</i> a <b>test</b>!";
        Shine::StringType expectedMarkup = "<root><font face=\"times\"><ch value=\"this\" /></font><ch value=\" \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source = "<font color=\"#FF00FF\">this</font> <i>is</i> a <b>test</b>!";
        Shine::StringType expectedMarkup = "<root><font color=\"#FF00FF\"><ch value=\"this\" /></font><ch value=\" \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source = "<font color=\"#FF00FF\" face=\"times\">this</font> <i>is</i> a <b>test</b>!";
        Shine::StringType expectedMarkup = "<root><font color=\"#FF00FF\" face=\"times\"><ch value=\"this\" /></font><ch value=\" \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source = "<font face=\"times\" color=\"#FF00FF\" >this</font> <i>is</i> a <b>test</b>!";
        Shine::StringType expectedMarkup = "<root><font face=\"times\" color=\"#FF00FF\" ><ch value=\"this\" /></font><ch value=\" \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source = "<font face=\"times\">this <i>is</i> a <b>test</b>!</font>";
        Shine::StringType expectedMarkup = "<root><font face=\"times\"><ch value=\"this \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></font></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source = "<font color=\"#FF00FF\">this <i>is</i> a <b>test</b>!</font>";
        Shine::StringType expectedMarkup = "<root><font color=\"#FF00FF\"><ch value=\"this \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></font></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source = "<font color=\"#FF00FF\" face=\"times\">this <i>is</i> a <b>test</b>!</font>";
        Shine::StringType expectedMarkup = "<root><font color=\"#FF00FF\" face=\"times\"><ch value=\"this \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></font></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source = "<font face=\"times\" color=\"#FF00FF\" >this <i>is</i> a <b>test</b>!</font>";
        Shine::StringType expectedMarkup = "<root><font face=\"times\" color=\"#FF00FF\" ><ch value=\"this \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></font></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    ///////////////////////////////////////////////////////////////
    // Font tags with no attributes: expect failure
    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source = "<font>this</font> <i>is</i> a <b>test</b>!";
        Shine::StringType expectedMarkup = "<root><font><ch value=\"this\" /></font><ch value=\" \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source = "this <i><font>is</font></i> a <b>test</b>!";
        Shine::StringType expectedMarkup = "<root><ch value=\"this \" /><i><font><ch value=\"is\" /></font></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source = "this <i>is</i> a <b><font>test</font></b>!";
        Shine::StringType expectedMarkup = "<root><ch value=\"this \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><font><ch value=\"test\" /></font></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    ///////////////////////////////////////////////////////////////
    // Font tags with unrecognized attributes: expect failure
    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source = "<font bad=\"1\">this</font> <i>is</i> a <b>test</b>!";
        Shine::StringType expectedMarkup = "<root><font bad=\"1\"><ch value=\"this\" /></font><ch value=\" \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source = "this <i><font bad=\"1\">is</font></i> a <b>test</b>!";
        Shine::StringType expectedMarkup = "<root><ch value=\"this \" /><i><font bad=\"1\"><ch value=\"is\" /></font></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source = "this <i>is</i> a <b><font bad=\"1\">test</font></b>!";
        Shine::StringType expectedMarkup = "<root><ch value=\"this \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><font bad=\"1\"><ch value=\"test\" /></font></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    ///////////////////////////////////////////////////////////////
    // Spacing tests
    {
        const Shine::StringType rawCharData("this  is a test!");
        Shine::StringType source = "this  <i>is</i> a <b>test</b>!";
        Shine::StringType expectedMarkup = "<root><ch value=\"this  \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData(" thisis a test!");
        Shine::StringType source = " this<i>is</i> a <b>test</b>!";
        Shine::StringType expectedMarkup = "<root><ch value=\" this\" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData(" this is a test!");
        Shine::StringType source = " this <i>is</i> a <b>test</b>!";
        Shine::StringType expectedMarkup = "<root><ch value=\" this \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source = "<b>this</b> <i>is</i> a <b>test</b>!";
        Shine::StringType expectedMarkup = "<root><b><ch value=\"this\" /></b><ch value=\" \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("this  is a test!");
        Shine::StringType source = "<b>this</b>  <i>is</i> a <b>test</b>!";
        Shine::StringType expectedMarkup = "<root><b><ch value=\"this\" /></b><ch value=\"  \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("this                         is a test!");
        Shine::StringType source = "<b>this</b>                         <i>is</i> a <b>test</b>!";
        Shine::StringType expectedMarkup = "<root><b><ch value=\"this\" /></b><ch value=\"                         \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source = "<b>this</b><i></i> <i>is</i> a <b>test</b>!";
        Shine::StringType expectedMarkup = "<root><b><ch value=\"this\" /></b><i></i><ch value=\" \" /><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source = "<b>this</b> <i></i><i>is</i> a <b>test</b>!";
        Shine::StringType expectedMarkup = "<root><b><ch value=\"this\" /></b><ch value=\" \" /><i></i><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source = "<b>this</b><i></i> <b></b><i>is</i> a <b>test</b>!";
        Shine::StringType expectedMarkup = "<root><b><ch value=\"this\" /></b><i></i><ch value=\" \" /><b></b><i><ch value=\"is\" /></i><ch value=\" a \" /><b><ch value=\"test\" /></b><ch value=\"!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    ///////////////////////////////////////////////////////////////
    // Spacing tests: character escaping
    {
        const Shine::StringType rawCharData("&  1");
        Shine::StringType source = "&amp;  1";
        Shine::StringType expectedMarkup = "<root><ch value=\"&amp;  1\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("& 1");
        Shine::StringType source = "&amp; 1";
        Shine::StringType expectedMarkup = "<root><ch value=\"&amp; 1\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("& ");
        Shine::StringType source = "&amp; ";
        Shine::StringType expectedMarkup = "<root><ch value=\"&amp; \" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData(" &");
        Shine::StringType source = " &amp;";
        Shine::StringType expectedMarkup = "<root><ch value=\" &amp;\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData(" & ");
        Shine::StringType source = " &amp; ";
        Shine::StringType expectedMarkup = "<root><ch value=\" &amp; \" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("&1 ");
        Shine::StringType source = "&amp;1 ";
        Shine::StringType expectedMarkup = "<root><ch value=\"&amp;1 \" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("& 1");
        Shine::StringType source = "&amp; 1";
        Shine::StringType expectedMarkup = "<root><ch value=\"&amp; 1\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("1&");
        Shine::StringType source = "1&amp;";
        Shine::StringType expectedMarkup = "<root><ch value=\"1&amp;\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("1& ");
        Shine::StringType source = "1&amp; ";
        Shine::StringType expectedMarkup = "<root><ch value=\"1&amp; \" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("1 &");
        Shine::StringType source = "1 &amp;";
        Shine::StringType expectedMarkup = "<root><ch value=\"1 &amp;\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData(" 1&");
        Shine::StringType source = " 1&amp;";
        Shine::StringType expectedMarkup = "<root><ch value=\" 1&amp;\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("&1 &");
        Shine::StringType source = "&amp;1 &amp;";
        Shine::StringType expectedMarkup = "<root><ch value=\"&amp;1 &amp;\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("& 1&");
        Shine::StringType source = "&amp; 1&amp;";
        Shine::StringType expectedMarkup = "<root><ch value=\"&amp; 1&amp;\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("& 1& ");
        Shine::StringType source = "&amp; 1&amp; ";
        Shine::StringType expectedMarkup = "<root><ch value=\"&amp; 1&amp; \" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("&1 & ");
        Shine::StringType source = "&amp;1 &amp; ";
        Shine::StringType expectedMarkup = "<root><ch value=\"&amp;1 &amp; \" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData(" &1 & ");
        Shine::StringType source = " &amp;1 &amp; ";
        Shine::StringType expectedMarkup = "<root><ch value=\" &amp;1 &amp; \" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("& 1 &");
        Shine::StringType source = "&amp; 1 &amp;";
        Shine::StringType expectedMarkup = "<root><ch value=\"&amp; 1 &amp;\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("& &");
        Shine::StringType source = "&amp; &amp;";
        Shine::StringType expectedMarkup = "<root><ch value=\"&amp; &amp;\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    ///////////////////////////////////////////////////////////////
    // Pass-thru tests
    {
        Shine::StringType source = " this is a test!";
        Shine::StringType expectedMarkup = "<root><ch value=\" this is a test!\" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        Shine::StringType source = "this is a test! ";
        Shine::StringType expectedMarkup = "<root><ch value=\"this is a test! \" /></root>";

        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        const Shine::StringType rawCharData("this is a test!");
        Shine::StringType source;
        Shine::StringType expectedMarkup = "<root></root>";;
        Shine::StringType markupTarget;
        InsertMarkup(source, markupTarget);
        AZ_Assert(expectedMarkup == markupTarget, "Test failed");

        Shine::StringType target;
        TextMarkup::CopyCharData(rawCharData, target);
        AZ_Assert(rawCharData == target, "Test failed");
    }

    ///////////////////////////////////////////////////////////////
    // Bad xml tests
    {
        Shine::StringType source = "<this <i>is</i> a <b>test</b>!";
        Shine::StringType expectedMarkup = "<root><this <i>is</i> a <b>test</b>!</root>";

        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        Shine::StringType source = "<<this <i>is</i> a <b>test</b>!";
        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        Shine::StringType source = "this<badtag></badtag> <i>is</i> a <b>test</b>!";
        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        Shine::StringType source = "<       this <i>is</i> a <b>test</b>!";
        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        Shine::StringType source = "<>this <i>is</i> a <b>test</b>!";
        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    {
        Shine::StringType source = "<font face=\"times\">this</font></b> <i>is</i> a <b>test</b>!";
        Shine::StringType target;
        TextMarkup::CopyCharData(source, target);
        AZ_Assert(source == target, "Test failed");
    }

    AnchorTagTests();
}
#endif
