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

// Description : Stores and splits the log string into types  message  warning  error
//               Functionality to adds the latest log to a textbox  and to query some info about the log


using System.Collections.Generic;
using System.Drawing;
using System.Runtime.InteropServices;

namespace RemoteConsole
{
    class LogBuffer
    {
        private System.Text.StringBuilder sb = new System.Text.StringBuilder(4048);
        private List<Color> selColors = new List<Color>();
        private List<int> selIndex = new List<int>();
        private List<Color> colors = new List<Color>();
				private System.Text.StringBuilder sbSingleLine = new System.Text.StringBuilder(256);

				private List<string> linesMessage = new List<string>();
				private List<string> linesError = new List<string>();
				private List<string> linesWarning = new List<string>();

        public LogBuffer()
        {
            colors.Add(Color.Black);
            colors.Add(Color.White);
            colors.Add(Color.Blue);
            colors.Add(Color.Green);
            colors.Add(Color.Red);
            colors.Add(Color.LightBlue);
            colors.Add(Color.Yellow);
            colors.Add(Color.Magenta);
            colors.Add(Color.Orange);
            colors.Add(Color.LightGray);
        }

				public void clear()
				{
					sb.Clear();
					linesMessage.Clear();
					linesError.Clear();
					linesWarning.Clear();
				}

				public List<string> GetLinesInBuffer(EMessageType msgType)
				{
					switch (msgType)
					{
						case EMessageType.eMT_Message: return linesMessage;
						case EMessageType.eMT_Warning: return linesWarning;
						case EMessageType.eMT_Error: return linesError;
					}

					return null;
				}

				public int GetNumLinesInBuffer()
				{
					return linesMessage.Count + linesWarning.Count + linesError.Count;
				}

				public bool IsBufferEmpty()
				{
					return sb.Length == 0;
				}

				public bool IsTagInBuffer(string tag)
				{
					return sb.ToString().Contains(tag);
				}

        public void addLine(string line, Color color, EMessageType msgType)
        {
						sbSingleLine.Clear();

            // color input
            selColors.Add(color);
            int len = sb.Length;
            selIndex.Add(len);

            // append new line
            int f = 0;
            for (int i = 0; i < line.Length && line[i] != '\0'; ++i)
            {
							if (line[i] == '$' && i + 1 < line.Length && line[i + 1] >= '0' && line[i + 1] <= '9')
							{
								int colIdx = int.Parse(line[i + 1].ToString());
								selColors.Add(colors[colIdx]);
								selIndex.Add(len + i - f);
								f += 2;
								i++;
							}
							else
							{
								sb.Append(line.Substring(i, 1));
								sbSingleLine.Append(line.Substring(i, 1));
							}
            }
						sb.Append("\n");
						sbSingleLine.Append("\n");
						switch (msgType)
						{
							case EMessageType.eMT_Message: linesMessage.Add(sbSingleLine.ToString()); break;
							case EMessageType.eMT_Warning: linesWarning.Add(sbSingleLine.ToString()); break;
							case EMessageType.eMT_Error: linesError.Add(sbSingleLine.ToString()); break;
							default: linesMessage.Add(sbSingleLine.ToString()); break;

						}
        }

        [DllImport("user32.dll")]
				public static extern int SendMessage(System.IntPtr hWnd, System.Int32 wMsg, bool wParam, System.Int32 lParam);
        private const int WM_SETREDRAW = 11;

				public void addToTextBox(System.Windows.Forms.RichTextBox b, bool bClear = true, bool bFlatColor = false)
        {
            if (sb.Length > 0)
            {
				if (!Common.IsRunningOnMono())
					SendMessage(b.Handle, WM_SETREDRAW, false, 0);

                int selStart = b.SelectionStart;
                int selLen = b.SelectionLength;

                int length = b.TextLength;
                int lenAdd = sb.Length;
								string text = sb.ToString();
								b.AppendText(text);
								if (bClear)
								{
									sb.Clear();
								}
                selIndex.Add(lenAdd);

								if (bFlatColor == true)
								{
									b.SelectionColor = Color.White;
								}
								else
								{
									for (int i = 0; i < selColors.Count; ++i)
									{
										b.Select(length + selIndex[i], selIndex[i + 1] - selIndex[i]);
										b.SelectionColor = selColors[i];
									}
								}

                if (selStart == length)
                {
                    selStart = length + lenAdd;
                    selLen = 0;
                }

                b.Select(selStart, selLen);
                b.ScrollToCaret();

                selColors.Clear();
                selIndex.Clear();
				if (!Common.IsRunningOnMono())
					SendMessage(b.Handle, WM_SETREDRAW, true, 0);
                b.Refresh();
            }
        }
    }
}
