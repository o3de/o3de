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

﻿namespace RemoteConsole
{
	partial class FormSettings
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
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.cbMode = new System.Windows.Forms.ComboBox();
			this.groupBox2 = new System.Windows.Forms.GroupBox();
			this.btEditFilters = new System.Windows.Forms.Button();
			this.btEditMenus = new System.Windows.Forms.Button();
			this.groupBox3 = new System.Windows.Forms.GroupBox();
			this.cbDebugMidi = new System.Windows.Forms.CheckBox();
			this.groupBox1.SuspendLayout();
			this.groupBox2.SuspendLayout();
			this.groupBox3.SuspendLayout();
			this.SuspendLayout();
			// 
			// groupBox1
			// 
			this.groupBox1.Controls.Add(this.cbMode);
			this.groupBox1.Location = new System.Drawing.Point(12, 12);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(229, 53);
			this.groupBox1.TabIndex = 1;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "Operation Mode";
			// 
			// cbMode
			// 
			this.cbMode.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.cbMode.FormattingEnabled = true;
			this.cbMode.Location = new System.Drawing.Point(6, 19);
			this.cbMode.Name = "cbMode";
			this.cbMode.Size = new System.Drawing.Size(207, 21);
			this.cbMode.TabIndex = 0;
			this.cbMode.SelectedIndexChanged += new System.EventHandler(this.cbMode_SelectedIndexChanged);
			// 
			// groupBox2
			// 
			this.groupBox2.Controls.Add(this.btEditFilters);
			this.groupBox2.Controls.Add(this.btEditMenus);
			this.groupBox2.Location = new System.Drawing.Point(12, 71);
			this.groupBox2.Name = "groupBox2";
			this.groupBox2.Size = new System.Drawing.Size(229, 61);
			this.groupBox2.TabIndex = 2;
			this.groupBox2.TabStop = false;
			this.groupBox2.Text = "Edit Configuration Files";
			// 
			// btEditFilters
			// 
			this.btEditFilters.FlatAppearance.BorderSize = 2;
			this.btEditFilters.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
			this.btEditFilters.Location = new System.Drawing.Point(138, 20);
			this.btEditFilters.Name = "btEditFilters";
			this.btEditFilters.Size = new System.Drawing.Size(75, 35);
			this.btEditFilters.TabIndex = 1;
			this.btEditFilters.Text = "Filters";
			this.btEditFilters.UseVisualStyleBackColor = true;
			this.btEditFilters.Click += new System.EventHandler(this.btEditFilters_Click);
			// 
			// btEditMenus
			// 
			this.btEditMenus.FlatAppearance.BorderSize = 2;
			this.btEditMenus.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
			this.btEditMenus.Location = new System.Drawing.Point(6, 20);
			this.btEditMenus.Name = "btEditMenus";
			this.btEditMenus.Size = new System.Drawing.Size(75, 35);
			this.btEditMenus.TabIndex = 0;
			this.btEditMenus.Text = "Menus";
			this.btEditMenus.UseVisualStyleBackColor = true;
			this.btEditMenus.Click += new System.EventHandler(this.btEditMenus_Click);
			// 
			// groupBox3
			// 
			this.groupBox3.Controls.Add(this.cbDebugMidi);
			this.groupBox3.Location = new System.Drawing.Point(18, 139);
			this.groupBox3.Name = "groupBox3";
			this.groupBox3.Size = new System.Drawing.Size(200, 51);
			this.groupBox3.TabIndex = 3;
			this.groupBox3.TabStop = false;
			this.groupBox3.Text = "Debug";
			// 
			// cbDebugMidi
			// 
			this.cbDebugMidi.AutoSize = true;
			this.cbDebugMidi.Location = new System.Drawing.Point(7, 20);
			this.cbDebugMidi.Name = "cbDebugMidi";
			this.cbDebugMidi.Size = new System.Drawing.Size(45, 17);
			this.cbDebugMidi.TabIndex = 0;
			this.cbDebugMidi.Text = "Midi";
			this.cbDebugMidi.UseVisualStyleBackColor = true;
			this.cbDebugMidi.CheckedChanged += new System.EventHandler(this.cbDebugMidi_CheckedChanged);
			// 
			// FormSettings
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(251, 202);
			this.Controls.Add(this.groupBox3);
			this.Controls.Add(this.groupBox2);
			this.Controls.Add(this.groupBox1);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
			this.HelpButton = true;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "FormSettings";
			this.Text = "Settings";
			this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.FormSettings_FormClosing);
			this.groupBox1.ResumeLayout(false);
			this.groupBox2.ResumeLayout(false);
			this.groupBox3.ResumeLayout(false);
			this.groupBox3.PerformLayout();
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.ComboBox cbMode;
		private System.Windows.Forms.GroupBox groupBox2;
		private System.Windows.Forms.Button btEditFilters;
		private System.Windows.Forms.Button btEditMenus;
		private System.Windows.Forms.GroupBox groupBox3;
		private System.Windows.Forms.CheckBox cbDebugMidi;
	}
}