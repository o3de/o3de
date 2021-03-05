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
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace RemoteConsole
{
	public partial class FormSliders : Form
	{
		DelSendCommandToTarget delSendCommand;

		public FormSliders()
		{
			InitializeComponent();

			lvSliders.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);
			lvSliders.AutoResizeColumns(ColumnHeaderAutoResizeStyle.HeaderSize);
		}

		public void SetEntries(List<ParamsFileInfo.CEntry> entries, DelSendCommandToTarget callabck)
		{
			lvSliders.Items.Clear();

			foreach (var entry in entries)
			{
				// Populate List View
				ListViewItem lvi = new ListViewItem(entry.Name);
				//lvi.BackColor = System.Drawing.Color.Aqua;
				lvi.Tag = entry;

				lvSliders.Items.Add(lvi);		
			}

			// select first element
			if (entries.Count > 0)
			{
				lvSliders.SelectedIndices.Add(0);
				lvSliders.Select();
			}

			delSendCommand = callabck;
		}

		private void SendSliderCommand(bool forceTextBoxValue = false)
		{
			if (delSendCommand != null)
			{
				ListView.SelectedListViewItemCollection items = this.lvSliders.SelectedItems;
				if (items.Count > 0)
				{
					ParamsFileInfo.CEntry entry = (ParamsFileInfo.CEntry)items[0].Tag;
					ParamsFileInfo.CEntryTag tag = entry.GenerateEntryTag();

					// Update UI
					if (forceTextBoxValue)
					{
						entry.SliderParams.CurrentValue = float.Parse(rtbValue.Text);
						entry.SliderParams.CurrentValue = Common.Clamp(entry.SliderParams.CurrentValue, entry.SliderParams.Min, entry.SliderParams.Max);
					}
					else
					{
						float v = CalculateSliderValue(entry);
						entry.SliderParams.CurrentValue = Common.Clamp(v, entry.SliderParams.Min, entry.SliderParams.Max);
					}
					rtbValue.Text = entry.SliderParams.CurrentValue.ToString();
					UpdateSliderPosition();

					// Send commands
					tag.ModCmds = new List<string>();
					for (int k = 0; k < tag.Entry.Data.Count; ++k)
					{
						string s = rtbValue.Text.Replace(',', '.');
						tag.ModCmds.Add(tag.Entry.Data[k].Replace("#", s));
					}
					delSendCommand(tag);
				}
			}
		}
		private void trackBar1_Scroll(object sender, EventArgs e)
		{
			// Send command
			SendSliderCommand();
		}

		private void UpdateSliderPosition()
		{

		}

		private void lvSliders_SelectedIndexChanged(object sender, EventArgs e)
		{
			ListView.SelectedListViewItemCollection items = this.lvSliders.SelectedItems;
			if (items.Count > 0)
			{
				ParamsFileInfo.CEntry entry = (ParamsFileInfo.CEntry)items[0].Tag;
				setSliderUi(entry);
				foreach (var s in entry.Data)
					rtbDescription.Text = s;
			}
		}

		private float CalculateSliderValue(ParamsFileInfo.CEntry entry)
		{
			if (entry.SliderParams != null)
			{
				if (entry.SliderParams.ForceInt)
				{
					return (float)trackBar1.Value;
				}
				else
				{
					float t = (float)(trackBar1.Value) / 1000f;
					float v = entry.SliderParams.Lerp(t);
					return v;
				}
			}
			return 0f;
		}

		private void setSliderUi(ParamsFileInfo.CEntry entry)
		{
			lblName.Text = entry.Name;

			if (entry.SliderParams != null)
			{
				if (entry.SliderParams.ForceInt)
				{
					trackBar1.TickFrequency = (int)(entry.SliderParams.Delta);
					trackBar1.Minimum = (int)entry.SliderParams.Min;
					trackBar1.Maximum = (int)entry.SliderParams.Max;
					trackBar1.Value = (int)entry.SliderParams.CurrentValue;
				}
				else
				{
					float size = entry.SliderParams.Max - entry.SliderParams.Min;
					float w = size > 0f ? size / (entry.SliderParams.Delta > 0 ? entry.SliderParams.Delta : 1f) : 1f;
					trackBar1.TickFrequency = (int)(1000f / w);
					trackBar1.Minimum = 0;
					trackBar1.Maximum = 1000;
					int v = (int)(1000f * (entry.SliderParams.CurrentValue - entry.SliderParams.Min) / size);
					trackBar1.Value = Common.Clamp(v, 0, 1000);
				}

				lblMin.Text = entry.SliderParams.Min.ToString();
				lblMax.Text = entry.SliderParams.Max.ToString();

				rtbValue.Text = entry.SliderParams.CurrentValue.ToString();
			}
		}

		private void rtbValue_KeyUp(object sender, KeyEventArgs e)
		{
			if (e.KeyCode == Keys.Enter)
			{
				try
				{
					float v = float.Parse(rtbValue.Text, System.Globalization.CultureInfo.InvariantCulture);
					SendSliderCommand(true);
				}
				catch (System.Exception)
				{
				}
			}
			else if (e.KeyCode == Keys.Escape)
			{
				((RichTextBox)sender).Undo();
			}
		}

		private void FormSliders_FormClosing(object sender, FormClosingEventArgs e)
		{
			e.Cancel = true;
			this.Hide();
		}
	}
}
