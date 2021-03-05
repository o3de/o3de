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

namespace RemoteConsole
{
	partial class FormButtons
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing)
		{
			if (disposing && (components != null))
			{
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.lvToggles = new System.Windows.Forms.ListView();
			this.SuspendLayout();
			// 
			// lvToggles
			// 
			this.lvToggles.CheckBoxes = true;
			this.lvToggles.Dock = System.Windows.Forms.DockStyle.Fill;
			this.lvToggles.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
			this.lvToggles.Location = new System.Drawing.Point(0, 0);
			this.lvToggles.MultiSelect = false;
			this.lvToggles.Name = "lvToggles";
			this.lvToggles.Size = new System.Drawing.Size(283, 354);
			this.lvToggles.TabIndex = 3;
			this.lvToggles.UseCompatibleStateImageBehavior = false;
			this.lvToggles.View = System.Windows.Forms.View.Details;
			this.lvToggles.ItemChecked += new System.Windows.Forms.ItemCheckedEventHandler(this.lvToggles_ItemChecked);
			// 
			// FormButtons
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(283, 354);
			this.Controls.Add(this.lvToggles);
			this.Name = "FormButtons";
			this.Text = "Toggle Buttons";
			this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.FormButtons_FormClosing_1);
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.ListView lvToggles;
	}
}