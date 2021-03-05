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

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Windows.Forms;

namespace RemoteConsole
{
	public partial class FormButtons : Form
	{
		DelSendCommandToTarget delSendCommand;

		public FormButtons()
		{
			InitializeComponent();
		}

		public void SetEntries(List<ParamsFileInfo.CEntry> entries, DelSendCommandToTarget callabck)
		{
			delSendCommand = null;
			lvToggles.Clear();

			lvToggles.FullRowSelect = true;
			lvToggles.GridLines = true;
			lvToggles.Location = new System.Drawing.Point(3, 3);
			lvToggles.UseCompatibleStateImageBehavior = false;
			lvToggles.View = System.Windows.Forms.View.Details;
		//	lvToggles.ContextMenuStrip = contextMenu;

			ColumnHeader columnHeader0 = new ColumnHeader();
			columnHeader0.Text = "Toggle";
			columnHeader0.Width = -1;

			lvToggles.Columns.AddRange(new ColumnHeader[] { columnHeader0 });
			lvToggles.Width = -1; // autosize it to the width of the widest element in the column

			foreach (var entry in entries)
			{
				if (entry.ToggleParams != null)
				{
					// Populate List View
					ListViewItem lvi = new ListViewItem(entry.Name);
					lvi.Tag = entry;
					//lvi.ToolTipText = 

					ListViewGroup group;
					int gIdx = GetGroupIndex(entry.ToggleParams.GroupName);
					if (gIdx < 0)
					{
						group = new ListViewGroup(entry.ToggleParams.GroupName);
						lvToggles.Groups.Add(group);
					}
					else
					{
						group = lvToggles.Groups[gIdx];
					}
										
					group.Items.Add(lvi);
					
					lvToggles.Items.Add(lvi);
				}
			}

			lvToggles.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);
			lvToggles.AutoResizeColumns(ColumnHeaderAutoResizeStyle.HeaderSize);
			delSendCommand = callabck;
		}

		// ------------------------------------------------------------------------

		private void SendSliderCommand(ParamsFileInfo.CEntry entry, int value)
		{
			if (delSendCommand != null)
			{
				ParamsFileInfo.CEntryTag tag = entry.GenerateEntryTag();

				// Send commands
				tag.ModCmds = new List<string>();
				for (int k = 0; k < tag.Entry.Data.Count; ++k)
				{
					string s = value.ToString();
					tag.ModCmds.Add(tag.Entry.Data[k].Replace("#", s));
				}
				delSendCommand(tag);
			}
		}

		// ------------------------------------------------------------------------

		private int GetGroupIndex(string name)
		{
			int index = -1;
			for (int i = 0; i < lvToggles.Groups.Count; ++i)
			{
				ListViewGroup g = lvToggles.Groups[i];
				if (g.Header == name)
				{
					index = i;
					break;
				}
			}

			return index;
		}

		// ------------------------------------------------------------------------

		private void lvToggles_ItemChecked(object sender, ItemCheckedEventArgs e)
		{
			if (e.Item.Tag != null && delSendCommand != null)
			{
				ParamsFileInfo.CEntry entry = (ParamsFileInfo.CEntry)e.Item.Tag;
				if (entry.ToggleParams != null)
				{
					int value = e.Item.Checked ? entry.ToggleParams.On : entry.ToggleParams.Off;
					SendSliderCommand(entry, value);
				}
			}
		}

		// ------------------------------------------------------------------------

		private void FormButtons_FormClosing_1(object sender, FormClosingEventArgs e)
		{
			e.Cancel = true;
			this.Hide();
		}
	}
}
