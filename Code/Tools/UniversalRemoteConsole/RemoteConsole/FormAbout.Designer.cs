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
	partial class FormAbout
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
			this.panel1 = new System.Windows.Forms.Panel();
			this.label1 = new System.Windows.Forms.Label();
			this.label3 = new System.Windows.Forms.Label();
			this.label2 = new System.Windows.Forms.Label();
			this.rtbMidi = new System.Windows.Forms.RichTextBox();
			this.label4 = new System.Windows.Forms.Label();
			this.SuspendLayout();
			// 
			// panel1
			// 
			this.panel1.BackgroundImage = global::RemoteConsole.Properties.Resources.fractal;
			this.panel1.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
			this.panel1.Location = new System.Drawing.Point(1, 2);
			this.panel1.Name = "panel1";
			this.panel1.Size = new System.Drawing.Size(228, 147);
			this.panel1.TabIndex = 0;
			// 
			// label1
			// 
			this.label1.AutoSize = true;
			this.label1.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.label1.ForeColor = System.Drawing.SystemColors.HotTrack;
			this.label1.Location = new System.Drawing.Point(12, 152);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(199, 17);
			this.label1.TabIndex = 1;
			this.label1.Text = "Universal Remote Console";
			this.label1.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
			// 
			// label3
			// 
			this.label3.AutoSize = true;
			this.label3.Location = new System.Drawing.Point(107, 178);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(109, 13);
			this.label3.TabIndex = 3;
			this.label3.Text = "Author: Dario Sancho";
			this.label3.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
			// 
			// label2
			// 
			this.label2.AutoSize = true;
			this.label2.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.label2.Location = new System.Drawing.Point(12, 178);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(94, 13);
			this.label2.TabIndex = 4;
			this.label2.Text = "(c) Crytek 2014";
			this.label2.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
			// 
			// rtbMidi
			// 
			this.rtbMidi.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
			this.rtbMidi.BackColor = System.Drawing.SystemColors.Control;
			this.rtbMidi.BorderStyle = System.Windows.Forms.BorderStyle.None;
			this.rtbMidi.Location = new System.Drawing.Point(15, 231);
			this.rtbMidi.Name = "rtbMidi";
			this.rtbMidi.ReadOnly = true;
			this.rtbMidi.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.None;
			this.rtbMidi.Size = new System.Drawing.Size(201, 57);
			this.rtbMidi.TabIndex = 5;
			this.rtbMidi.TabStop = false;
			this.rtbMidi.Text = "The Midi code is based on Tom Lokovic\'s [https://code.google.com/p/midi-dot-net/]" +
    " project, adapted by Ramon Villadomat and Dario Sancho.";
			this.rtbMidi.LinkClicked += new System.Windows.Forms.LinkClickedEventHandler(this.rtbMidi_LinkClicked);
			// 
			// label4
			// 
			this.label4.AutoSize = true;
			this.label4.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.label4.Location = new System.Drawing.Point(62, 210);
			this.label4.Name = "label4";
			this.label4.Size = new System.Drawing.Size(92, 13);
			this.label4.TabIndex = 6;
			this.label4.Text = "Acknoledgements";
			this.label4.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
			// 
			// FormAbout
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(228, 300);
			this.Controls.Add(this.label4);
			this.Controls.Add(this.rtbMidi);
			this.Controls.Add(this.label2);
			this.Controls.Add(this.label3);
			this.Controls.Add(this.label1);
			this.Controls.Add(this.panel1);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
			this.Name = "FormAbout";
			this.Text = "About";
			this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.FormAbout_FormClosing);
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.Panel panel1;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.Label label3;
		private System.Windows.Forms.Label label2;
		private System.Windows.Forms.RichTextBox rtbMidi;
		private System.Windows.Forms.Label label4;
	}
}