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

// Description : Control to display the log data  it could be either a LISTVIEW or a RICHTEXTBOX


using System.Windows.Forms;

namespace RemoteConsole
{
	class LogDisplayControl
	{
		private ListView lv = null;
		private RichTextBox rtb = null;

		public enum EType
		{
			eT_ListView = 0,
			eT_RichTextBox
		}

		public Control GetControl() 
		{ 
			if (lv != null)
				return lv;
			else
				return rtb;
		}

		public LogDisplayControl(EType controlType, ContextMenuStrip contextMenu, int textColor)
		{
			if (controlType == EType.eT_ListView)
			{
				lv = new ListView();
				lv.Dock = System.Windows.Forms.DockStyle.Fill;
				lv.FullRowSelect = true;
				lv.GridLines = true;
				lv.Location = new System.Drawing.Point(3, 3);
				lv.Size = new System.Drawing.Size(797, 118);
				lv.UseCompatibleStateImageBehavior = false;
				lv.View = System.Windows.Forms.View.Details;
				lv.ContextMenuStrip = contextMenu;

				lv.Columns.Add("I");
				lv.Columns.Add("Description");
				lv.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);
				lv.AutoResizeColumns(ColumnHeaderAutoResizeStyle.HeaderSize);
				lv.ForeColor = System.Drawing.Color.FromArgb(textColor);
			}
			else
			{
				rtb = new RichTextBox();
				rtb.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
							| System.Windows.Forms.AnchorStyles.Left)
							| System.Windows.Forms.AnchorStyles.Right)));
				rtb.BackColor = System.Drawing.SystemColors.WindowText;
				rtb.ContextMenuStrip = contextMenu;
				rtb.ForeColor = System.Drawing.Color.Silver;
				rtb.Location = new System.Drawing.Point(3, 6);
				rtb.ReadOnly = true;
				rtb.Size = new System.Drawing.Size(700, 233);
				rtb.TabIndex = 1;
				rtb.Text = "";
				rtb.ScrollBars = RichTextBoxScrollBars.Both;
				rtb.Dock = DockStyle.Fill;
				rtb.ForeColor = System.Drawing.Color.FromArgb(textColor);
			}

		}

		public int GetNumItems()
		{
			return (lv != null) ? lv.Items.Count : rtb.Lines.Length;
		}

		public void AddLine(string line)
		{
			if (lv != null)
			{
				ListViewItem lvi = new ListViewItem(lv.Items.Count.ToString());
				lvi.SubItems.Add(line);
				lv.Items.Add(lvi);
				lv.EnsureVisible(lv.Items.Count - 1);
			}
			else
			{
				rtb.AppendText(line);
			}
		}

		public void Clear()
		{
			if (lv != null)
				lv.Items.Clear();
			else if (rtb != null)
				rtb.Clear();
		}

		public void Copy()
		{
			if (lv != null)
			{
				Clipboard.Clear();
				string fullText = "";
				foreach (ListViewItem item in lv.SelectedItems)
				{
					fullText += item.SubItems[1].Text;// +Environment.NewLine;
				}
				Clipboard.SetText(fullText);
			}
			else if (rtb != null)
			{
				rtb.Copy();
			}
		}

		public void SelectAll()
		{
			if (lv != null)
			{
				foreach (ListViewItem item in lv.Items)
				{
					item.Selected = true;
				}
			}
			else
			{
				rtb.SelectAll();
			}
		}

	}
}
