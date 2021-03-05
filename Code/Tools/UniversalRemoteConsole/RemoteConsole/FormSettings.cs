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
	public partial class FormSettings : Form
	{
		private DelSettingsChange delSettingsChange;
		private CSettings settings = new CSettings();

		public FormSettings()
		{
			InitializeComponent();

			cbMode.Items.Clear();
			cbMode.Items.Add("Online  [Full]");
			cbMode.Items.Add("Online  [Commands Only]");
			cbMode.Items.Add("Online  [Files Only]");
			cbMode.SelectedIndex = 0;
			cbMode.Select();
		}

		public void SetSettingsChangeDelegate(DelSettingsChange callback)
		{
			delSettingsChange = callback;
		}

		private void WarnAboutMissingFiles(string fileName)
		{
			string message =
						 "Missing file: " + fileName + "\nPlease please the file in the given directory (same folder as the executable).";
			const string caption = "Missing Config File!";
			var result = MessageBox.Show(message, caption,
																	 MessageBoxButtons.OK,
																	 MessageBoxIcon.Error);
		}

		private void btEditMenus_Click(object sender, EventArgs e)
		{
			System.IO.FileInfo fileInfo = new System.IO.FileInfo(Common.MenusFileFullPath);

			if (fileInfo.Exists == true)
			{
				System.Diagnostics.Process.Start(/*"notepad.exe", */Common.MenusFileFullPath);
			}
			else
			{
				WarnAboutMissingFiles(Common.MenusFileFullPath);
			}
		}

		private void btEditFilters_Click(object sender, EventArgs e)
		{
			System.IO.FileInfo fileInfo = new System.IO.FileInfo(Common.FiltersFileFullPath);

			if (fileInfo.Exists == true)
			{
				System.Diagnostics.Process.Start(/*"notepad.exe", */Common.FiltersFileFullPath);
			}
			else
			{
				WarnAboutMissingFiles(Common.FiltersFileFullPath);
			}
		}

		private void FormSettings_FormClosing(object sender, FormClosingEventArgs e)
		{
			e.Cancel = true;
			this.Hide();
		}

		private void cbMode_SelectedIndexChanged(object sender, EventArgs e)
		{
			UpdateSettings();
			if (delSettingsChange != null) delSettingsChange(settings);
		}

		private void cbDebugMidi_CheckedChanged(object sender, EventArgs e)
		{
			UpdateSettings();
			if (delSettingsChange != null) 	delSettingsChange(settings);
		}

		private void UpdateSettings()
		{
			settings.DebugMidi = cbDebugMidi.Checked;
			if (cbMode.SelectedIndex >= 0)
				settings.Mode = (CSettings.EMode)cbMode.SelectedIndex;
		}
	}
}
