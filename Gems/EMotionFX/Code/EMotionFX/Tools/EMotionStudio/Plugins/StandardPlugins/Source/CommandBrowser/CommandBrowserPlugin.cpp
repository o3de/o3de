/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/*

// include required headers
#include "CommandBrowserPlugin.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <QWebView>


namespace EMStudio
{

// constructor
CommandBrowserPlugin::CommandBrowserPlugin() : EMStudio::DockWidgetPlugin()
{
    mWebView = nullptr;
}


// destructor
CommandBrowserPlugin::~CommandBrowserPlugin()
{
}


// clone the log window
EMStudioPlugin* CommandBrowserPlugin::Clone()
{
    CommandBrowserPlugin* newPlugin = new CommandBrowserPlugin();
    return newPlugin;
}


// init after the parent dock window has been created
bool CommandBrowserPlugin::Init()
{
    //LogInfo("Initializing command browser window.");
    //QWebSettings::globalSettings()->setAttribute(QWebSettings::JavascriptEnabled, true);
    mWebView = new QWebView( mDock );
    mWebView->setObjectName( "QWebView" );
    mWebView->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
    mWebView->settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true );
    mDock->SetContents( mWebView );
    GenerateCommandList();
    return true;
}


void CommandBrowserPlugin::GenerateCommandList()
{
    CommandSystem::CommandManager* manager = GetCommandManager();

    uint32 i;
    const uint32 numCommands = manager->GetNumRegisteredCommands();

    // generate the html header
    QString html;
    html = "<html>";
    html += "<head>";
    html += "<STYLE TYPE=\"text/css\">\n";
    html += "<!--\n";
    html += "BODY\n";
    html += "{\n";
    html += "   color: orange;\n";
    html += "   background-color: #1E1E1E;\n";
    html += "   font-family: sans-serif;\n";
    html += "   font-size: 8pt;\n";
    html += "}\n";
    html += "A:link{color:white}\n";
    //html += "A:visited{color:yellow}\n";

    html += "TABLE\n";
    html += "{\n";
    html += "   border-width: 1px;\n";
    html += "   border-collapse: collapse;\n";
    html += "   border-color: #333333;\n";
    html += "   border-style: solid;\n";
    html += "   padding: 1px;\n";
    html += "   color: orange;\n";
    html += "   text-align: \"left\";\n";
    html += "   background-color: #1E1E1E;\n";
    html += "   font-family: sans-serif;\n";
    html += "   font-size: 8pt;\n";
    html += "}\n";

    html += "TH\n";
    html += "{\n";
    html += "   border-width: 1px;\n";
    html += "   border-collapse: collapse;\n";
    html += "   border-color: #333333;\n";
    html += "   border-style: solid none solid none;\n";
    html += "   background-color: orange;\n";
    html += "   padding: 1px;\n";
    html += "   color: black;\n";
    html += "   text-align: \"left\";\n";
    html += "   font-family: sans-serif;\n";
    html += "   font-size: 8pt;\n";
    html += "}\n";


    html += "-->\n";
    html += "</STYLE>\n";

    html += "<script language=\"javascript\" type=\"text/javascript\">\n";
    html += "var opened = new Array();\n";
    html += "function showCommandInfo(id)\n";
    html += "{\n";
    html += "    if (opened[id] == false)\n";
    html += "    {\n";
    html += "        switch(id)\n";
    html += "        {\n";

    // generate the command info
    for (i=0; i<numCommands; ++i)
    {
        MCore::Command* command = manager->GetCommand(i);

        QString infoString;

        QString newString;
        infoString += "<table border=1 width=90%>";
        infoString += "<tr align=left>";
        infoString += "<th>Name</th>";
        infoString += "<th>Type</th>";
        infoString += "<th>Required</th>";
        infoString += "<th>Default Value</th>";
        infoString += "<th>Description</th>";
        infoString += "</tr>";

        // for all parameters
        const uint32 numParams = command->GetSyntax().GetNumParameters();
        for (uint32 p=0; p<numParams; ++p)
        {
            infoString += "<tr>";
            newString.sprintf("<td>%s</td>", command->GetSyntax().GetParamName(p));                     infoString += newString;
            newString.sprintf("<td>%s</td>", command->GetSyntax().GetParamTypeString(p));               infoString += newString;
            newString.sprintf("<td>%s</td>", command->GetSyntax().GetParamRequired(p) ? "Yes" : "No");  infoString += newString;
            newString.sprintf("<td>%s</td>", command->GetSyntax().GetDefaultValue(p).c_str());         infoString += newString;
            newString.sprintf("<td>%s</td>", command->GetSyntax().GetParamDescription(p));              infoString += newString;
            infoString += "</tr>";
        }
        infoString += "</table>";

        QString description = command->GetDescription();
        if (description.isEmpty() == false)
        {
            infoString += "<table border=1 width=90%>";
            infoString += "<tr align=left>";
            infoString += "<th>Description</th>";
            infoString += "</tr>";
            infoString += "<tr><td>";
            description.replace("\n", "<br>");
            infoString += description;
            infoString += "</td></tr>";
            infoString += "</table>";
        }

        newString.sprintf("case \"%s\": document.getElementById(id).innerHTML = \"%s<br>\"; break;\n", command->GetName(), infoString);
        html += newString;
    }

    // continue the script code
    html += "        }\n";
    html += "        opened[id] = true;\n";
    html += "   }\n";
    html += "   else\n";
    html += "   {\n";
    html += "       document.getElementById(id).innerHTML = \"\";\n";
    html += "       opened[id] = false;\n";
    html += "   }\n";
    html += "}\n";
    html += "</script>\n";
    html += "</head>\n";
    html += "<body>\n";

    // init all commands as closed
    html += "<script type=\"text/javascript\">\n";
    for (i=0; i<numCommands; ++i)
    {
        MCore::Command* command = manager->GetCommand(i);
        QString newString;
        newString.sprintf("   opened['%s'] = false;\n", command->GetName());
        html += newString;
    }
    html += "</script>\n";

    // write the alphabet
    html += "<font size=\"2pt\" face=\"Verdana\">\n";
    MCore::Command* prevCommand = nullptr;
    for (i=0; i<numCommands; ++i)
    {
        MCore::Command* command = manager->GetCommand(i);

        // find out if we need to show the leading character
        bool newFirstCharacter = false;
        if (prevCommand == nullptr)
            newFirstCharacter = true;
        else
            if (prevCommand->GetName()[0] != command->GetName()[0])
                newFirstCharacter = true;

        // display the first character as link
        if (newFirstCharacter)
        {
            const char character = command->GetName()[0];
            QString newString;
            newString.sprintf("<a style=\"color:orange;\" href=\"#POS_%c\">%c</a> ", character, character);
            html += newString;
        }

        prevCommand = command;
    }
    html += "</font>";
    html += "<br>";
    html += "<hr color=#333333 size=1px>";
    html += "<br>\n";


    // show all commands as links
    prevCommand = nullptr;
    for (i=0; i<numCommands; ++i)
    {
        MCore::Command* command = manager->GetCommand(i);

        // find out if we need to show the leading character
        bool newFirstCharacter = false;
        if (prevCommand == nullptr)
            newFirstCharacter = true;
        else
            if (prevCommand->GetName()[0] != command->GetName()[0])
                newFirstCharacter = true;

        if (newFirstCharacter)
        {
            // if this isn't the first command
            if (i != 0)
            {
                html += "<br>";
                //html += "<hr color=#333333 size=1px>";
                html += "<br>";
            }

            QString newString;
            newString.sprintf("<a name=\"POS_%c\">", command->GetName()[0]);
            html += newString;

            html += "<font size=\"4pt\" face=\"Verdana\">\n";
            html += (char)command->GetName()[0];
            html += "</font>";

            html += "<br>";
            html += "<hr color=#333333 size=1px>";
            //html += "<br>";
            //html += "<br>";
        }

        QString newString;
        newString.sprintf("<a href=\"javascript:void(0)\" onClick=\"showCommandInfo('%s');\">%s</a><br>\n", command->GetName(), command->GetName());
        html += newString;
        newString.sprintf("<div id=\"%s\"></div>\n",  command->GetName());
        html += newString;

        prevCommand = command;
    }
    html += "</body></html>\n";


    // set the html code
    mWebView->setHtml( html );
}

}   // namespace EMStudio
*/
