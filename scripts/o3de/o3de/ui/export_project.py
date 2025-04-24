#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import tkinter as tk
import tkinter.tix as tkp
from tkinter import ttk, filedialog, messagebox

import os
import pathlib
import platform

from . import multiple_file_picker
from . import multiple_entry

EXPORT_SCRIPTS_PATH = pathlib.Path(__file__).parent.parent.parent / 'ExportScripts'
assert EXPORT_SCRIPTS_PATH.is_dir(), "Missing expected ExportScripts path"


class MainWindow(tkp.Tk):
    """
    Main Widget to manage the settings for Project Export based on an export config
    """

    def __init__(self, export_config, is_sdk):
        """
        Initialize the MainWindow Tk widget

        :param export_config:   The export_config that is relevant to the project (or global setting)
        :param is_sdk:          Flag indicating if the engine is an SDK engine (vs source-build)
        """
        super().__init__()

        self.title("Project Export Settings")

        self.export_config = export_config

        self.is_sdk = is_sdk

        if self.export_config.is_global:
            self.title("Project Export Settings (Global)")
        else:
            self.title(f"Project Export Settings ({self.export_config.project_name})")

        self.config_key_entry_map = {}

        self.min_label_width = 30
        self.min_entry_width = 40
        self.field_padding = 2

        self.columnconfigure(0, weight=1)

        self.main_frame = tk.Frame(self)
        self.main_frame.columnconfigure(0, weight=1)
        self.main_frame.grid(sticky=tk.NSEW)
        self.tool_tip = tkp.Balloon(self.main_frame)

        if not self.is_sdk:
            self.init_source_engine_build_options(self.main_frame)

        self.init_project_build_options(self.main_frame)

        self.init_asset_bundling_options(self.main_frame)

        self.init_platform_tabs(self.main_frame)

        self.init_okay_cancel_buttons(self.main_frame)
        
        self.eval('tk::PlaceWindow . center')

    def add_simple_text_entry(self, parent: tk.Frame or tk.LabelFrame, config_key: str, label_text: str,
                              entry_read_only: bool = False) -> None:
        """
        Add a simple text entry (label and an edit box) to the parent frame widget

        :param parent:          The parent frame widget (using grid management)
        :param config_key:      The settings config key to attach to this entry
        :param label_text:      The text for the label of this entry
        :param entry_read_only: Is this a read-only entry?
        """

        assert config_key not in self.config_key_entry_map.keys(), f'Duplicate config key {config_key}'

        default_value = self.export_config.get_value(config_key, '')
        settings = self.export_config.get_settings_description(config_key)
        tooltip_description = settings.description

        label = tk.Label(parent, text=label_text, anchor=tk.W, width=self.min_label_width)
        label.grid(column=0, sticky=tk.W, padx=self.field_padding, pady=self.field_padding)
        self.tool_tip.bind_widget(label, balloonmsg=tooltip_description)

        row_line = label.grid_info().get("row")

        entry_var = tk.StringVar(value=default_value)
        entry = tk.Entry(parent, justify='right', state=tk.DISABLED if entry_read_only else tk.NORMAL,
                         textvariable=entry_var, width=self.min_entry_width)

        self.tool_tip.bind_widget(entry, balloonmsg=tooltip_description)
        entry.grid(row=row_line, column=1, sticky=tk.EW, padx=self.field_padding, pady=self.field_padding)

        self.config_key_entry_map[config_key] = (label_text, default_value, entry_var, settings, entry)

    def add_dropdown_entry(self, parent: tk.Frame or tk.LabelFrame, config_key: str, label_text: str,
                           choices: list[str]) -> None:
        """
        Add a labeled drop-down entry list to the parent frame widget

        :param parent:          The parent frame widget (using grid management)
        :param config_key:      The settings config key to attach to this entry
        :param label_text:      The text for the label of this entry
        :param choices:         List of possible entries for the drop down
        """

        assert config_key not in self.config_key_entry_map.keys(), f'Duplicate config key {config_key}'

        default_value = self.export_config.get_value(config_key)
        settings = self.export_config.get_settings_description(config_key)
        tooltip_description = settings.description

        label = tk.Label(parent, text=label_text, anchor=tk.W, width=self.min_label_width)
        label.grid(column=0, sticky=tk.W, padx=self.field_padding, pady=self.field_padding)
        self.tool_tip.bind_widget(label, balloonmsg=tooltip_description)

        row_line = label.grid_info().get("row")

        assert len(choices) > 0, "choices must not be empty"
        if default_value == '':
            default_value = choices[0]

        assert default_value in choices, f"{default_value} is not in the list of choices ({','.join(choices)})"

        entry_var = tk.StringVar(value=default_value)
        entry = tk.OptionMenu(parent, entry_var,  *choices)
        entry.grid(row=row_line, column=1, sticky=tk.E, padx=self.field_padding, pady=self.field_padding)
        self.tool_tip.bind_widget(entry, balloonmsg=tooltip_description)

        self.config_key_entry_map[config_key] = (label_text,default_value, entry_var, settings, entry)

    def add_labeled_checkbox(self, parent: tk.Frame or tk.LabelFrame, config_key: str, label_text: str,
                             row_number: int = None, column_number: int = None) -> None:
        """
        Add a check box to the parent frame widget

        :param parent:          The parent frame widget (using grid management)
        :param config_key:      The settings config key to attach to this entry
        :param label_text:      The text for the label of this entry
        :param row_number:      Override the row number in the parent grid container
        :param column_number:   Override the column number in the parent grid container
        """

        assert config_key not in self.config_key_entry_map.keys(), f'Duplicate config key {config_key}'

        default_value = 1 if self.export_config.get_value(config_key).lower() == 'true' else 0
        settings = self.export_config.get_settings_description(config_key)
        tooltip_description = settings.description

        check_button_var = tk.IntVar(value=default_value)
        button = tk.Checkbutton(parent, text=label_text, variable=check_button_var,onvalue=1, offvalue=0)
        button.grid(row=row_number, column=column_number, sticky=tk.W)
        self.tool_tip.bind_widget(button, balloonmsg=tooltip_description)

        self.config_key_entry_map[config_key] = (label_text, default_value, check_button_var, settings, button)

    def add_multi_file_entry(self, parent: tk.Frame or tk.LabelFrame, config_key: str, label_text: str,
                             multifile_file_select: bool = False, entry_read_only: bool = False, file_filters=None) -> None:
        """
        Add a labeled text box that represents a list of multiple files

        :param parent:                  The parent frame widget (using grid management)
        :param config_key:              The settings config key to attach to this entry
        :param label_text:              The text for the label of this entry
        :param multifile_file_select:   Option to use the custom multi-file select (true) or the simple single file select
        :param entry_read_only: Is this a read-only entry text?
        :param file_filters:            The file filters to pass to the open file dialog
        """

        assert config_key not in self.config_key_entry_map.keys(), f'Duplicate config key {config_key}'

        default_value = self.export_config.get_value(config_key, '')
        settings = self.export_config.get_settings_description(config_key)
        tooltip_description = settings.description

        label = tk.Label(parent, text=label_text, anchor=tk.W, width=self.min_label_width)
        label.grid(column=0, sticky=tk.W, padx=self.field_padding, pady=self.field_padding)
        self.tool_tip.bind_widget(label, balloonmsg=tooltip_description)
        row_line = label.grid_info().get("row")

        entry_and_button_frame = tk.Frame(parent)
        entry_and_button_frame.columnconfigure(0, weight=1)
        entry_and_button_frame.columnconfigure(1, weight=0)
        entry_and_button_frame.grid(row=row_line, column=1, sticky=tk.EW, padx=self.field_padding, pady=self.field_padding)

        entry_var = tk.StringVar(value=default_value)
        entry = tk.Entry(entry_and_button_frame, textvariable=entry_var, justify='right',
                         state=tk.DISABLED if entry_read_only else tk.NORMAL, width=self.min_entry_width)
        entry.grid(column=0, sticky=tk.EW)
        self.tool_tip.bind_widget(entry, balloonmsg=tooltip_description)
        sub_row_line = entry.grid_info().get("row")

        if multifile_file_select:
            button_add = tk.Button(entry_and_button_frame, text="...", width=3,
                                   command=lambda: MainWindow.open_multi_file_dialog(parent=parent, str_var=entry_var, file_filters=file_filters))
        else:
            button_add = tk.Button(entry_and_button_frame, text="...", width=3,
                                   command=lambda: MainWindow.open_single_file_dialog(parent=parent, str_var=entry_var, file_filters=file_filters, starting_dir=EXPORT_SCRIPTS_PATH))
        button_add.grid(row=sub_row_line, column=1, sticky=tk.E, padx=self.field_padding, pady=self.field_padding)
        self.tool_tip.bind_widget(button_add, balloonmsg=tooltip_description)

        self.config_key_entry_map[config_key] = (label_text, default_value, entry_var, settings, entry)

    def add_multi_text_entry(self, parent: tk.Frame or tk.LabelFrame, config_key: str, label_text: str,
                             entry_read_only: bool = False):
        """
        Add a labeled text box that represents a list of multiple values

        :param parent:          The parent frame widget (using grid management)
        :param config_key:      The settings config key to attach to this entry
        :param label_text:      The text for the label of this entry
        :param entry_read_only: Is this a read-only entry text?
        """

        assert config_key not in self.config_key_entry_map.keys(), f'Duplicate config key {config_key}'

        default_value = self.export_config.get_value(config_key, '')
        settings = self.export_config.get_settings_description(config_key)
        tooltip_description = settings.description

        label = tk.Label(parent, text=label_text, anchor=tk.W, width=self.min_label_width)
        label.grid(column=0,sticky=tk.W, padx=self.field_padding, pady=self.field_padding)
        row_line = label.grid_info().get("row")
        self.tool_tip.bind_widget(label, balloonmsg=tooltip_description)

        entry_and_button_frame = tk.Frame(parent)
        entry_and_button_frame.columnconfigure(0, weight=1)
        entry_and_button_frame.columnconfigure(1, weight=0)
        entry_and_button_frame.grid(row=row_line, column=1, sticky=tk.EW, padx=self.field_padding, pady=self.field_padding)

        entry_var = tk.StringVar(value=default_value)
        entry = tk.Entry(entry_and_button_frame, textvariable=entry_var, justify='right', state=tk.DISABLED if entry_read_only else tk.NORMAL, width=self.min_entry_width)
        entry.grid(row=row_line, column=0, sticky=tk.EW, padx=self.field_padding, pady=self.field_padding)
        self.tool_tip.bind_widget(entry, balloonmsg=tooltip_description)
        sub_row_line = entry.grid_info().get("row")

        button_add = tk.Button(entry_and_button_frame, text="...", width=3, command=lambda : MainWindow.open_multi_text_dialog(parent=parent, str_var=entry_var))
        button_add.grid(row=sub_row_line, column=1, sticky=tk.E)
        self.tool_tip.bind_widget(button_add, balloonmsg=tooltip_description)

        self.config_key_entry_map[config_key] = (label_text, default_value, entry_var, settings, entry)

    @staticmethod
    def open_multi_file_dialog(parent, str_var: tk.StringVar, file_filters):
        """
        Open the custom dialog to select multiple files

        :param parent:          The parent widget
        :param str_var:         The tk.StringVar to get and set the values
        :param file_filters:    The file filters to pass to the open file dialog
        """
        value = str_var.get()
        result = multiple_file_picker.Dialog(parent=parent, file_filters=file_filters,initial_list=value).get_result()
        str_var.set(result)

    @staticmethod
    def open_single_file_dialog(parent, str_var: tk.StringVar, file_filters, starting_dir):
        result = filedialog.askopenfilename(filetypes=file_filters, initialdir=starting_dir)
        filename_only = os.path.basename(result)
        str_var.set(filename_only)

    @staticmethod
    def open_multi_text_dialog(parent, str_var: tk.StringVar):
        """
        Open the custom dialog to manage multiple text entries

        :param parent:          The parent widget
        :param str_var:         The tk.StringVar to get and set the values
        """
        value = str_var.get()
        result = multiple_entry.Dialog(parent=parent, input_value=value).get_result()
        str_var.set(result)

    def init_archive_settings_frame(self, parent) -> None:
        """
        Initialize + Configure the Archive Settings frame

        :param parent:  The parent widget grid container
        """
        general_settings_frame = tk.LabelFrame(parent, text="Archive Settings")
        general_settings_frame.columnconfigure(0, weight=0)
        general_settings_frame.columnconfigure(1, weight=1)
        general_settings_frame.grid(padx=4, pady=4, sticky=tk.NSEW)

        archive_format_choices = ['none', 'zip', 'gzip', 'bz2', 'xz']
        self.add_dropdown_entry(parent=general_settings_frame,
                                label_text='Archive Format',
                                config_key='archive.output.format',
                                choices=archive_format_choices)

        self.add_multi_text_entry(parent=general_settings_frame,
                                  label_text="Game Project File Patterns",
                                  config_key='additional.game.project.file.pattern.to.copy')

        self.add_multi_text_entry(parent=general_settings_frame,
                                  label_text="Server Project File Patterns",
                                  config_key='additional.server.project.file.pattern.to.copy')

        self.add_multi_text_entry(parent=general_settings_frame,
                                  label_text="Output Path",
                                  config_key='option.default.output.path')

    def init_source_engine_build_options(self, parent):
        """
        Initialize + Configure the Toolchain Build Options

        :param parent:  The parent widget grid container
        """
        tool_chain_build_options_frame = tk.LabelFrame(parent, text="Toolchain Build Options")
        tool_chain_build_options_frame.columnconfigure(0, weight=0)
        tool_chain_build_options_frame.columnconfigure(1, weight=1)
        tool_chain_build_options_frame.grid(padx=8, sticky=tk.EW)
        build_config_choices = ['debug', 'profile', 'release']

        self.add_labeled_checkbox(parent=tool_chain_build_options_frame,
                                  label_text='Build Toolchain',
                                  config_key='option.build.tools',
                                  column_number=0)

        self.add_dropdown_entry(parent=tool_chain_build_options_frame,
                                label_text='Tool Build Configuration',
                                config_key='tool.build.config',
                                choices=build_config_choices)

        self.add_simple_text_entry(parent=tool_chain_build_options_frame,
                                   label_text='Toolchain Build Path',
                                   config_key='default.build.tools.path')

    def init_project_build_options(self, parent):
        """
        Initialize + Configure the Project Build Options

        :param parent:  The parent widget grid container
        """
        project_build_options_frame = tk.LabelFrame(parent, text="Project Build Options")
        project_build_options_frame.columnconfigure(0, weight=0)
        project_build_options_frame.columnconfigure(1, weight=1)
        project_build_options_frame.grid(padx=8, sticky=tk.EW)

        build_config_choices = ['profile', 'release']
        if not self.is_sdk:
            build_config_choices.insert(0, 'debug')

        self.add_dropdown_entry(parent=project_build_options_frame,
                                label_text='Project Build Configuration',
                                config_key='project.build.config',
                                choices=build_config_choices)
        
    def init_asset_bundling_options(self, parent):
        """
        Initialize + Configure the Asset/Bundling Options

        :param parent:  The parent widget grid container
        """
        project_asset_bundling_options_frame = tk.LabelFrame(parent, text="Asset/Bundling Options")
        project_asset_bundling_options_frame.columnconfigure(0, weight=0)
        project_asset_bundling_options_frame.columnconfigure(1, weight=1)
        project_asset_bundling_options_frame.grid(padx=4, sticky=tk.EW)

        
        self.add_labeled_checkbox(parent=project_asset_bundling_options_frame,
                                  label_text='Build Assets',
                                  config_key='option.build.assets',
                                  row_number=5,
                                  column_number=0)

        self.add_labeled_checkbox(parent=project_asset_bundling_options_frame,
                                  label_text='Fail on Build Assets Errors',
                                  config_key='option.fail.on.asset.errors',
                                  row_number=5,
                                  column_number=1)

        self.add_multi_file_entry(parent=project_asset_bundling_options_frame,
                                  label_text="Seed List File Paths",
                                  config_key='seedlist.paths',
                                  multifile_file_select=True,
                                  file_filters=[("Seed List File", "*.seed")])

        self.add_multi_file_entry(parent=project_asset_bundling_options_frame,
                                  label_text="Seed (Asset) File Paths",
                                  config_key='seedfile.paths',
                                  multifile_file_select=True,
                                  file_filters=[("All Files", "*.*")])

        self.add_multi_text_entry(parent=project_asset_bundling_options_frame,
                                  label_text="Level Names",
                                  config_key='default.level.names')
        
        self.add_simple_text_entry(parent=project_asset_bundling_options_frame,
                                   label_text='Max Size',
                                   config_key='max.size')

        self.add_simple_text_entry(parent=project_asset_bundling_options_frame,
                                   label_text='Asset Bundling Path',
                                   config_key='asset.bundling.path')
    def init_pc_tab(self, parent):
        pc_tab = ttk.Frame(parent)
        pc_build_options_frame = tk.LabelFrame(pc_tab, text="Build Options")
        pc_build_options_frame.grid(sticky='nsew')
        os_name = 'Mac' if platform.system() == 'Darwin' else platform.system()
        self.add_labeled_checkbox(parent=pc_build_options_frame,
                                  label_text='Build Game Launcher',
                                  config_key='option.build.game.launcher',
                                  row_number=0,
                                  column_number=0)

        self.add_labeled_checkbox(parent=pc_build_options_frame,
                                  label_text='Build Unified Launcher',
                                  config_key='option.build.unified.launcher',
                                  row_number=0,
                                  column_number=1)

        self.add_labeled_checkbox(parent=pc_build_options_frame,
                                  label_text="Build Server Launcher",
                                  config_key='option.build.server.launcher',
                                  row_number=1,
                                  column_number=0)

        self.add_labeled_checkbox(parent=pc_build_options_frame,
                                  label_text='Build Headless Server Launcher',
                                  config_key='option.build.headless.server.launcher',
                                  row_number=1,
                                  column_number=1)
        
        # For pre-built SDKs builds, the monolithic build are determined by whether or not the 
        # build configuration is profile (false) or release (true), so only display the option
        # for source builds
        if not self.is_sdk:
            self.add_labeled_checkbox(parent=pc_build_options_frame,
                                      label_text="Build Monolithic",
                                      config_key='option.build.monolithic')
        
        self.add_labeled_checkbox(parent=pc_build_options_frame,
                                  label_text='Allow Asset Processor Registry Overrides',
                                  config_key='option.allow.registry.overrides')
        
        self.add_simple_text_entry(parent=pc_build_options_frame,
                                   label_text=f'{os_name} Launcher Build Path',
                                   config_key='default.launcher.build.path')
        
        # iOS is only available on Mac (Darwin)
        if platform.system() == 'Darwin':
            self.add_simple_text_entry(parent=pc_build_options_frame,
                                       label_text='iOS Build Path',
                                       config_key='default.ios.build.path')
            
        self.init_archive_settings_frame(pc_tab)
        parent.add(pc_tab, text=platform.system())
    
    def init_android_tab(self, parent):
        android_tab = ttk.Frame(parent)
        android_build_options_frame = tk.LabelFrame(android_tab, text="Build Options")
        android_build_options_frame.grid(sticky='nsew')
        self.add_labeled_checkbox(parent=android_build_options_frame,
                                  label_text='Deploy to Android Device',
                                  config_key='option.android.deploy')
        
        asset_mode_choices = ['LOOSE', 'PAK']
        self.add_dropdown_entry(parent=android_build_options_frame,
                                label_text='Asset Mode Configuration',
                                config_key='option.android.assets.mode',
                                choices=asset_mode_choices)

        self.add_simple_text_entry(parent=android_build_options_frame,
                                   label_text='Android APK Build Path',
                                   config_key='default.android.build.path')
        parent.add(android_tab, text='Android')

    def init_platform_tabs(self, parent):
        platform_tabs = ttk.Notebook(parent)
        platform_tabs.grid_rowconfigure(0, weight=1) # this needed to be added
        platform_tabs.grid_columnconfigure(0, weight=1) # as did this
        platform_tabs.columnconfigure(0, weight=0)
        platform_tabs.columnconfigure(1, weight=1)
        platform_tabs.grid(sticky=tk.EW)

        self.init_pc_tab(platform_tabs)
        self.init_android_tab(platform_tabs)

    def init_okay_cancel_buttons(self, parent):
        """
        Initialize + Configure the Okay and Cancel buttons

        :param parent:  The parent widget grid container
        """
        okay_cancel_frame = tk.Frame(parent)
        okay_cancel_frame.grid(padx=4, pady=4, sticky=tk.E)

        button_okay = tk.Button(okay_cancel_frame, text="Save", width=8, command=self.on_ok)
        button_okay.grid(column=0, sticky=tk.E)
        okay_row = button_okay.grid_info().get("row")

        button_cancel = tk.Button(okay_cancel_frame, text="Cancel", width=8, command=self.on_cancel)
        button_cancel.grid(row=okay_row, column=1, sticky=tk.E)

    def on_ok(self):
        for key, item in self.config_key_entry_map.items():

            label_text = item[0]
            previous_value = item[1]
            current_value = item[2].get()
            settings_config = item[3]
            widget = item[4]

            if previous_value != current_value:

                if settings_config.is_boolean:
                    assert isinstance(item[2], tk.IntVar)
                    set_value = 'True' if current_value==1 else 'False'
                else:
                    assert isinstance(item[2], tk.StringVar)
                    set_value = current_value
                try:
                    self.export_config.set_config_value(key=key,
                                                        value=set_value,
                                                        validate_value=True,
                                                        show_log=False)
                except Exception as e:
                    messagebox.showerror("Error", f"Input for {label_text} is not a valid value: {e}")
                    widget.focus_set()
                    return

        self.destroy()

    def on_cancel(self):
        self.destroy()

    def configure_settings(self):
        self.mainloop()
