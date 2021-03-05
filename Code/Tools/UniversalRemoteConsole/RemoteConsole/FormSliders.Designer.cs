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
	partial class FormSliders
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
			System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(FormSliders));
			this.panelSlider = new System.Windows.Forms.Panel();
			this.layoutPanelSlider = new System.Windows.Forms.TableLayoutPanel();
			this.trackBar1 = new System.Windows.Forms.TrackBar();
			this.lblMax = new System.Windows.Forms.Label();
			this.lblMin = new System.Windows.Forms.Label();
			this.lblName = new System.Windows.Forms.Label();
			this.rtbValue = new System.Windows.Forms.TextBox();
			this.lvSliders = new System.Windows.Forms.ListView();
			this.columnHeader1 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
			this.rtbDescription = new System.Windows.Forms.RichTextBox();
			this.label1 = new System.Windows.Forms.Label();
			this.panelSlider.SuspendLayout();
			this.layoutPanelSlider.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.trackBar1)).BeginInit();
			this.SuspendLayout();
			// 
			// panelSlider
			// 
			this.panelSlider.Controls.Add(this.layoutPanelSlider);
			this.panelSlider.Location = new System.Drawing.Point(251, 12);
			this.panelSlider.Name = "panelSlider";
			this.panelSlider.Size = new System.Drawing.Size(300, 90);
			this.panelSlider.TabIndex = 0;
			// 
			// layoutPanelSlider
			// 
			this.layoutPanelSlider.ColumnCount = 3;
			this.layoutPanelSlider.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 15F));
			this.layoutPanelSlider.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 70F));
			this.layoutPanelSlider.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 15F));
			this.layoutPanelSlider.Controls.Add(this.trackBar1, 1, 1);
			this.layoutPanelSlider.Controls.Add(this.lblMax, 2, 1);
			this.layoutPanelSlider.Controls.Add(this.lblMin, 0, 1);
			this.layoutPanelSlider.Controls.Add(this.lblName, 1, 0);
			this.layoutPanelSlider.Controls.Add(this.rtbValue, 1, 2);
			this.layoutPanelSlider.Dock = System.Windows.Forms.DockStyle.Fill;
			this.layoutPanelSlider.Location = new System.Drawing.Point(0, 0);
			this.layoutPanelSlider.Name = "layoutPanelSlider";
			this.layoutPanelSlider.RowCount = 3;
			this.layoutPanelSlider.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
			this.layoutPanelSlider.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 40F));
			this.layoutPanelSlider.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
			this.layoutPanelSlider.Size = new System.Drawing.Size(300, 90);
			this.layoutPanelSlider.TabIndex = 1;
			// 
			// trackBar1
			// 
			this.trackBar1.Dock = System.Windows.Forms.DockStyle.Fill;
			this.trackBar1.Location = new System.Drawing.Point(48, 23);
			this.trackBar1.Name = "trackBar1";
			this.trackBar1.Size = new System.Drawing.Size(204, 34);
			this.trackBar1.TabIndex = 0;
			this.trackBar1.Scroll += new System.EventHandler(this.trackBar1_Scroll);
			// 
			// lblMax
			// 
			this.lblMax.AutoSize = true;
			this.lblMax.Dock = System.Windows.Forms.DockStyle.Fill;
			this.lblMax.Location = new System.Drawing.Point(258, 20);
			this.lblMax.Name = "lblMax";
			this.lblMax.Size = new System.Drawing.Size(39, 40);
			this.lblMax.TabIndex = 2;
			this.lblMax.Text = "1";
			this.lblMax.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
			// 
			// lblMin
			// 
			this.lblMin.AutoSize = true;
			this.lblMin.Dock = System.Windows.Forms.DockStyle.Fill;
			this.lblMin.Location = new System.Drawing.Point(3, 20);
			this.lblMin.Name = "lblMin";
			this.lblMin.Size = new System.Drawing.Size(39, 40);
			this.lblMin.TabIndex = 1;
			this.lblMin.Text = "0";
			this.lblMin.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
			// 
			// lblName
			// 
			this.lblName.AutoSize = true;
			this.lblName.Dock = System.Windows.Forms.DockStyle.Fill;
			this.lblName.Location = new System.Drawing.Point(48, 0);
			this.lblName.Name = "lblName";
			this.lblName.Size = new System.Drawing.Size(204, 20);
			this.lblName.TabIndex = 3;
			this.lblName.Text = "Slider Name";
			this.lblName.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
			// 
			// rtbValue
			// 
			this.rtbValue.Dock = System.Windows.Forms.DockStyle.Fill;
			this.rtbValue.Location = new System.Drawing.Point(48, 63);
			this.rtbValue.Name = "rtbValue";
			this.rtbValue.Size = new System.Drawing.Size(204, 20);
			this.rtbValue.TabIndex = 4;
			this.rtbValue.Text = "0";
			this.rtbValue.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
			this.rtbValue.KeyUp += new System.Windows.Forms.KeyEventHandler(this.rtbValue_KeyUp);
			// 
			// lvSliders
			// 
			this.lvSliders.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader1});
			this.lvSliders.FullRowSelect = true;
			this.lvSliders.GridLines = true;
			this.lvSliders.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
			this.lvSliders.HideSelection = false;
			this.lvSliders.Location = new System.Drawing.Point(13, 12);
			this.lvSliders.Name = "lvSliders";
			this.lvSliders.Size = new System.Drawing.Size(213, 184);
			this.lvSliders.TabIndex = 1;
			this.lvSliders.UseCompatibleStateImageBehavior = false;
			this.lvSliders.View = System.Windows.Forms.View.Details;
			this.lvSliders.SelectedIndexChanged += new System.EventHandler(this.lvSliders_SelectedIndexChanged);
			// 
			// columnHeader1
			// 
			this.columnHeader1.Text = "Select a Slider";
			// 
			// rtbDescription
			// 
			this.rtbDescription.BorderStyle = System.Windows.Forms.BorderStyle.None;
			this.rtbDescription.Enabled = false;
			this.rtbDescription.Location = new System.Drawing.Point(251, 136);
			this.rtbDescription.Name = "rtbDescription";
			this.rtbDescription.Size = new System.Drawing.Size(300, 60);
			this.rtbDescription.TabIndex = 2;
			this.rtbDescription.Text = "";
			// 
			// label1
			// 
			this.label1.AutoSize = true;
			this.label1.Location = new System.Drawing.Point(372, 120);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(59, 13);
			this.label1.TabIndex = 3;
			this.label1.Text = "Commands";
			// 
			// FormSliders
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(561, 208);
			this.Controls.Add(this.label1);
			this.Controls.Add(this.rtbDescription);
			this.Controls.Add(this.lvSliders);
			this.Controls.Add(this.panelSlider);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.Name = "FormSliders";
			this.Text = "Sliders";
			this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.FormSliders_FormClosing);
			this.panelSlider.ResumeLayout(false);
			this.layoutPanelSlider.ResumeLayout(false);
			this.layoutPanelSlider.PerformLayout();
			((System.ComponentModel.ISupportInitialize)(this.trackBar1)).EndInit();
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.Panel panelSlider;
		private System.Windows.Forms.TableLayoutPanel layoutPanelSlider;
		private System.Windows.Forms.TrackBar trackBar1;
		private System.Windows.Forms.Label lblMax;
		private System.Windows.Forms.Label lblMin;
		private System.Windows.Forms.Label lblName;
		private System.Windows.Forms.TextBox rtbValue;
		private System.Windows.Forms.ListView lvSliders;
		private System.Windows.Forms.RichTextBox rtbDescription;
		private System.Windows.Forms.ColumnHeader columnHeader1;
		private System.Windows.Forms.Label label1;
	}
}