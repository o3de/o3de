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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

namespace RemoteConsole
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
				[System.STAThread]
        static void Main()
        {
					System.Windows.Forms.Application.EnableVisualStyles();
					System.Windows.Forms.Application.SetCompatibleTextRenderingDefault(false);
					System.Windows.Forms.Application.Run(new MainForm());
        }
    }
}
