#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import os
import argparse
import copy
import time

import tkinter as tk
from tkinter import messagebox
from tkinter import filedialog

from config_data import ConfigData
from keystore_settings import KeystoreSettings
import discovery
from wait_dialog import WaitDialog
from keystore_generator import KeystoreGenerator
from project_generator import ProjectGenerator

DEFAULT_APG_CONFIG_FILE="apg_config.json"

class TkApp(tk.Tk):
    """
    This is the main UI of the Android Project Generator, known as APG for short.
    """
    def __init__(self, config: ConfigData, config_file_path: str = ""):
        super().__init__()
        self.title("Android Project Generator")
        # Display the main window wherever the mouse is located.
        x, y = self.winfo_pointerx(), self.winfo_pointery()
        self.geometry(f"+{x}+{y}")

        self._config = config
        self._config_file_path_var = tk.StringVar()
        self._config_file_path_var.set(config_file_path)

        self._init_load_save_ui()
        self._init_keystore_settings_ui()
        self._init_sdk_settings_ui()
        self._init_report_ui()


    def _init_load_save_ui(self):
        frame = tk.Frame(self)
        frame.pack()

        buttons_frame = tk.Frame(frame)
        buttons_frame.pack()
        btn = tk.Button(buttons_frame, text="Load Settings", command=self.on_load_settings_button)
        btn.pack(padx=20, side=tk.LEFT)
        btn = tk.Button(buttons_frame, text="Save Settings", command=self.on_save_settings_button)
        btn.pack(padx=20, side=tk.RIGHT)
        lbl = tk.Label(frame, textvariable=self._config_file_path_var)
        lbl.pack()


    def _init_keystore_settings_ui(self):
        # Create a button widget with an event handler.
        keystore_frame = tk.Frame(self)
        keystore_frame.pack(expand=False, fill=tk.X)

        lbl = tk.Label(keystore_frame, text="============ Keystore Settings ============")
        lbl.pack()

        # Let's add the fields that make the Distinguished Name.
        dn_frame = tk.Frame(keystore_frame)
        dn_frame.pack(expand=True, fill=tk.X)
        self._init_keystore_distinguished_name_ui(dn_frame)

        # Now let's add the rest of the keystore fields.
        ks_data = self._config.keystore_settings
        self._keystore_validity_days_var = self._add_label_entry(keystore_frame, "Validity Days", ks_data.validity_days)[0]
        self._keystore_key_size_var = self._add_label_entry(keystore_frame, "Key Size", ks_data.key_size)[0]
        self._keystore_app_key_alias_var = self._add_label_entry(keystore_frame, "App Key Alias", ks_data.key_alias)[0]
        self._keystore_app_key_password_var = self._add_label_entry(keystore_frame, "App Key Password", ks_data.key_password)[0]
        self._keystore_keystore_password_var = self._add_label_entry(keystore_frame, "Keystore Password", ks_data.keystore_password)[0]
        self._keystore_file_var, _, row_frame =  self._add_label_entry(keystore_frame, "Keystore File", ks_data.keystore_file)
        btn = tk.Button(row_frame, text="...", command=self.on_select_keystore_file_button)
        btn.pack(side=tk.LEFT)

        btn = tk.Button(keystore_frame, text="Create Keystore", command=self.on_create_keystore_button)
        btn.pack()


    def _init_keystore_distinguished_name_ui(self, parent_frame: tk.Frame):
        tk.Label(parent_frame, text="Distinguished Name Settings:").pack(anchor=tk.W)
        spaceStr = "        "
        ks_data = self._config.keystore_settings
        self._dn_country_code_var = self._add_label_entry(parent_frame, f"{spaceStr}Country Code", ks_data.dn_country_code)[0]
        self._dn_company_var = self._add_label_entry(parent_frame, f"{spaceStr}Company (aka Organization)", ks_data.dn_organization)[0]
        self._dn_organizational_unit_var = self._add_label_entry(parent_frame, f"{spaceStr}Organizational Unit", ks_data.dn_organizational_unit)[0]
        self._dn_app_name_var = self._add_label_entry(parent_frame, f"{spaceStr}App Name (aka Common Name)", ks_data.dn_common_name)[0]


    def _init_sdk_settings_ui(self):
        sdk_frame = tk.Frame(self)
        sdk_frame.pack(expand=False, fill=tk.X)
        lbl = tk.Label(sdk_frame, text="========= Android SDK/NDK Settings ========")
        lbl.pack()
        cf = self._config
        self._android_ndk_version_var = self._add_label_entry(sdk_frame, "NDK Version", cf.android_ndk_version)[0]
        self._android_sdk_api_level_var = self._add_label_entry(sdk_frame, "SDK API Level", cf.android_sdk_api_level)[0]
        self._android_sdk_path_var, _, row_frame = self._add_label_entry(sdk_frame, "SDK Path", cf.android_sdk_path)
        btn = tk.Button(row_frame, text="...", command=self.on_select_sdk_path_button)
        btn.pack(side=tk.LEFT)
        # Add the meta quest project checkbox
        self._android_quest_flag_var = self._add_checkbox(sdk_frame, "This is a Meta Quest project", cf.is_meta_quest_project)[0]
        # Add the project generation button.
        btn = tk.Button(sdk_frame, text="Generate Project", command=self.on_generate_project_button)
        btn.pack()


    def _add_label_entry(self, parent_frame: tk.Frame, lbl_name: str, default_value: str = "") -> tuple[tk.StringVar, tk.Entry, tk.Frame]:
        """
        Returns the tuple (string_var, entry, row_frame),
        where  @string_var is the TK StringVar bound to the Entry widget,
               @entry is the Entry widget,
               @row_frame is the parent Frame that owns @entry widget.
        """
        row_frame = tk.Frame(parent_frame)
        row_frame.pack(padx=5, pady=2, expand=True, fill=tk.X)
        lbl = tk.Label(row_frame, text=lbl_name)
        lbl.pack(side=tk.LEFT, anchor=tk.W)
        entry = tk.Entry(row_frame, justify='right')
        entry.pack(side=tk.LEFT, expand=True, fill=tk.X)
        string_var = tk.StringVar()
        string_var.set(default_value)
        entry["textvariable"] = string_var
        return string_var, entry, row_frame


    def _add_checkbox(self, parent_frame: tk.Frame, lbl_name: str, default_value: bool = False) -> tuple[tk.BooleanVar, tk.Checkbutton, tk.Frame]:
        """
        Returns the tuple (BooleanVar, check_box, row_frame),
        where  @BooleanVar is the TK BooleanVar bound to the CheckBox widget,
               @check_box is the Checkbutton widget,
               @row_frame is the parent Frame that owns @check_box widget.
        """
        row_frame = tk.Frame(parent_frame)
        row_frame.pack(padx=5, pady=2, expand=True, fill=tk.X)
        bool_var = tk.BooleanVar()
        bool_var.set(default_value)
        # Create a Checkbutton widget and bind it to the variable.
        checkbutton = tk.Checkbutton(row_frame, text=lbl_name, variable=bool_var)
        checkbutton.pack(side=tk.LEFT, anchor=tk.W)
        return bool_var, checkbutton, row_frame


    def _init_report_ui(self):
        """
        Instanties the scrollable text widget where this app will report
        all the stdout and stderr string produced by all subprocess invoked
        by this application.
        """
        lbl = tk.Label(self, text="============ Operations Report ============")
        lbl.pack()
        self._report_text_widget = tk.Text(self, wrap=tk.WORD, borderwidth=2, relief=tk.SUNKEN)
        self._report_scrollbar_widget = tk.Scrollbar(self, orient=tk.VERTICAL, command=self._report_text_widget.yview)
        # Configure the Text widget and the Scrollbar widget.
        self._report_text_widget.configure(yscrollcommand=self._report_scrollbar_widget.set)
        self._report_text_widget.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self._report_scrollbar_widget.pack(side=tk.RIGHT, fill=tk.Y)


    def _get_time_now_str(self) -> str:
        """
        @returns The current local time as a formatted string. 
        """
        time_secs = time.time()
        time_st = time.localtime(time_secs)
        no_millis_str = time.strftime("%H:%M:%S", time_st)
        fractional_secs = int(str(time_secs).split(".")[1])
        return f"{no_millis_str}.{fractional_secs}"


    def _append_log_message(self, msg: str):
        """
        Append the msg with a timestamp to the report widget, and automatically scrolls to
        the bottom of the report.
        """
        timestamp_str = self._get_time_now_str()
        self._report_text_widget.insert(tk.END, f">>{timestamp_str}>>\n{msg}\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n")
        self._report_text_widget.see(tk.END) #scroll to the end.


    def create_keystore_settings_from_widgets(self) -> KeystoreSettings:
        """
        @returns A new KeystoreSettings object, where all the values are read from the
        current content in the UI text/entry fields.
        """
        ks = KeystoreSettings()
        ks.keystore_file = self._keystore_file_var.get()
        ks.keystore_password = self._keystore_keystore_password_var.get()
        ks.key_alias = self._keystore_app_key_alias_var.get()
        ks.key_password = self._keystore_app_key_password_var.get()
        ks.key_size = self._keystore_key_size_var.get()
        ks.validity_days = self._keystore_validity_days_var.get()
        ks.dn_common_name = self._dn_app_name_var.get()
        ks.dn_organizational_unit = self._dn_organizational_unit_var.get()
        ks.dn_organization = self._dn_company_var.get()
        ks.dn_country_code = self._dn_country_code_var.get()
        return ks


    def create_config_data_from_widgets(self) -> ConfigData:
        """
        @returns A new ConfigData object, where all the values are read from the
        current content in the UI text/entry fields.
        """
        config = copy.deepcopy(self._config)
        config.android_sdk_path = self._android_sdk_path_var.get()
        config.android_ndk_version = self._android_ndk_version_var.get()
        config.android_sdk_api_level = self._android_sdk_api_level_var.get()
        config.is_meta_quest_project = self._android_quest_flag_var.get()
        config.keystore_settings = self.create_keystore_settings_from_widgets()
        return config


    def update_widgets_from_keystore_settings(self, ks: KeystoreSettings):
        self._keystore_file_var.set(ks.keystore_file)
        self._keystore_keystore_password_var.set(ks.keystore_password)
        self._keystore_app_key_alias_var.set(ks.key_alias)
        self._keystore_app_key_password_var.set(ks.key_password)
        self._keystore_key_size_var.set(ks.key_size)
        self._keystore_validity_days_var.set(ks.validity_days)
        self._dn_app_name_var.set(ks.dn_common_name)
        self._dn_organizational_unit_var.set(ks.dn_organizational_unit)
        self._dn_company_var.set(ks.dn_organization)
        self._dn_country_code_var.set(ks.dn_country_code)


    def update_widgets_from_config(self, config: ConfigData):
        self._android_sdk_path_var.set(config.android_sdk_path)
        self._android_ndk_version_var.set(config.android_ndk_version)
        self._android_sdk_api_level_var.set(config.android_sdk_api_level)
        self._android_quest_flag_var.set(config.is_meta_quest_project)
        self.update_widgets_from_keystore_settings(config.keystore_settings)


    def on_load_settings_button(self):
        """
        Invoked when the user clicks the `Load Settings` button.
        """
        suggested_file_path = self._config_file_path_var.get()
        if suggested_file_path == "":
            suggested_file_path = os.path.join(self._config.project_path, DEFAULT_APG_CONFIG_FILE)
        initial_dir, initial_file = os.path.split(suggested_file_path)
        filename = filedialog.askopenfilename(
            initialdir=initial_dir,
            initialfile=initial_file,
            title="Load Settings",
            filetypes=[("JSON files", "*.json"), ("All files", "*")],
            defaultextension=".json",
            parent=self
            )
        if (not filename) or (not os.path.isfile(filename)):
            messagebox.showinfo("Invalid Settings File Path", f"The path {filename} is invalid!")
            return
        if not self._config.load_from_json_file(filename):
            messagebox.showerror("File I/O Error", f"Failed to read settings from file:\n{filename}")
            return
        self._config_file_path_var.set(filename)
        messagebox.showinfo("Success!", f"Current settings were loaded from file:\n{filename}")
        self.update_widgets_from_config(self._config)


    def on_save_settings_button(self):
        """
        Invoked when the user clicks the `Save Settings` button.
        """
        configData = self.create_config_data_from_widgets()
        suggested_file_path = self._config_file_path_var.get()
        if suggested_file_path == "":
            suggested_file_path = os.path.join(configData.project_path, DEFAULT_APG_CONFIG_FILE)
        initial_dir, initial_file = os.path.split(suggested_file_path)
        filename = filedialog.asksaveasfilename(
            initialdir=initial_dir,
            initialfile=initial_file,
            title="Save Settings",
            filetypes=[("JSON files", "*.json"), ("All files", "*")],
            defaultextension=".json",
            parent=self
            )
        if (filename is None) or (filename == ""):
            return # Cancelled by user.
        if not configData.save_to_json_file(filename):
            messagebox.showerror("File I/O Error", f"Failed to save settings to file {filename}")
        self._config = configData
        self._config_file_path_var.set(filename)
        messagebox.showinfo("Success!", f"Current settings were saved as file:\n{filename}")


    def _on_user_cancel_task(self):
        """
        This is a callback invoked by self._wait_dialog when the user
        decides to cancel the current operation.
        """
        self._wait_dialog = None
        self._cancel_current_operation()
        self._current_operation = None # Doesn't hurt to force this to None.


    def _tick_operation(self):
        """
        This function will be called periodically while there's an operation running in the background.
        """
        if self._current_operation and self._current_operation.is_finished():
            # The operation completed. Time to close the progress dialog
            # and report the results.
            if self._wait_dialog:
                 self._wait_dialog.close()
                 self._wait_dialog = None
            self._on_current_operation_finished()
            return
        # Keep ticking, until next time if the operation is finished or the user
        # decides to cancel the current operation.
        if self._wait_dialog:
            self._wait_dialog.on_tick(float(self._tick_delta_ms)/1000.0)
            self.after(self._tick_delta_ms, self._tick_operation)


    def _cancel_current_operation(self):
        """
        This one is called upon user request.
        """
        self._current_operation.cancel()
        report_msg = self._current_operation.get_report_msg()
        self._append_log_message(report_msg)
        messagebox.showinfo("Cancelled By User", f"{self._current_operation.get_basic_description()}\nwas cancelled by user!")
        self._current_operation = None


    def _on_current_operation_finished(self):
        # The current operation is finished. But it could have
        # finished with error or success. Let the user know the outcome.
        report_msg = self._current_operation.get_report_msg()
        self._append_log_message(report_msg)
        if self._current_operation.is_success():
            messagebox.showinfo("Success", f"{self._current_operation.get_basic_description()}\ncompleted succesfully!")
        else:
            messagebox.showerror("Error", f"{self._current_operation.get_basic_description()}\ncompleted with errors!")
        self._current_operation = None


    def on_select_keystore_file_button(self):
        """
        The user clicked the "..." button next to the `Keystore File` field, with the purpose
        of selecting a different keystore file.
        """
        suggested_file_path = self._keystore_file_var.get()
        if suggested_file_path == "":
            # If the user input data is empty, try the cached data.
            suggested_file_path = self._config.keystore_settings.keystore_file
        if suggested_file_path == "":
            # If the cached data is empty, let's try a default
            suggested_file_path = os.path.join(self._config.project_path, "app.keystore")

        initial_dir, initial_file = os.path.split(suggested_file_path)
        filename = filedialog.asksaveasfilename(
            initialdir=initial_dir,
            initialfile=initial_file,
            title="Select Keystore File",
            filetypes=[("keystore files", "*.keystore"), ("All files", "*")],
            defaultextension=".json",
            parent=self
            )
        if (not filename) or (filename == ""):
            messagebox.showerror("Error", f"Invalid Keystore File Path!")
            return
        self._keystore_file_var.set(filename)
        self._config.keystore_settings.keystore_file = filename


    def on_select_sdk_path_button(self):
        """
        The user clicked the "..." button next to the  `SDK Path` field, with the purpose
        of selecting a different Android SDK path.
        """
        configData = self.create_config_data_from_widgets()
        initial_dir = configData.android_sdk_path
        directory  = filedialog.askdirectory(
            initialdir=initial_dir,
            title="Pick Android SDK Location",
            parent=self
            )
        if (not directory ) or (directory == "") or (not os.path.isdir(directory)):
            messagebox.showerror("Invalid SDK Path", f"The path {directory} is invalid!")
            return
        if not discovery.could_be_android_sdk_directory(directory):
            messagebox.showwarning("Warning", f"The directory:\n{directory}\nDoesn't appear to be an Android SDK directory.")
        configData.android_sdk_path = directory
        self._config = configData 
        self.update_widgets_from_config(self._config)


    def on_create_keystore_button(self):
        """
        The user clicked the `Create Keystore` button. Will spawn the required tools to create the keystore.
        """
        ks = self.create_keystore_settings_from_widgets()
        if (not ks.keystore_file) or (ks.keystore_file == ""):
            messagebox.showerror("Error", f"A vaid `Keystore File` is required.")
            return
        if os.path.isfile(ks.keystore_file):
            result = messagebox.askyesno("Attention!", f"Do you want to replace the Keystore File:\n{ks.keystore_file}?")
            if not result:
                return
            else:
                # It's important to delete the existing keystore file, otherwise the java keytool will fail to replace it.
                try:
                    os.remove(ks.keystore_file)
                except Exception as err:
                    messagebox.showerror("Error", f"Failed to delete keystore file {ks.keystore_file}. Got Exception:\n{err}")
                    return
        self._config.keystore_settings = ks
        # Start the in-progress modal dialog.
        def _inner_cancel_cb():
            self._on_user_cancel_task()
        self._wait_dialog = WaitDialog(self, "Creating Keystore.\nThis operation takes around 5 seconds.", _inner_cancel_cb)
        self._tick_delta_ms = 250
        self.after(self._tick_delta_ms, self._tick_operation)
        # Instantiate and start the job.
        self._current_operation = KeystoreGenerator(self._config)
        self._current_operation.start()


    def on_generate_project_button(self):
        """
        The user clicked the `Generate Project` button. Will spawn the required tools to create the android project.
        """
        configData = self.create_config_data_from_widgets()
        # Make sure the keystore file exist.
        ks = configData.keystore_settings
        if (not ks.keystore_file) or (ks.keystore_file == ""):
            messagebox.showerror("Error", f"Can not generate an android project without a valid `Keystore File`.")
            return
        if not os.path.isfile(ks.keystore_file):
            messagebox.showerror("Error", f"The keystore file {ks.keystore_file} doesn't exist.\nPush the `Create Keystore` button to create it.")
            return
        # Start the in-progress modal dialog.
        def _inner_cancel_cb():
            self._on_user_cancel_task()
        self._wait_dialog = WaitDialog(self, "Generating the android project.\nThis operation takes around 30 seconds.", _inner_cancel_cb)
        self._tick_delta_ms = 250
        self.after(self._tick_delta_ms, self._tick_operation)
        # Instantiate and start the job.
        self._config = configData
        self._current_operation = ProjectGenerator(configData)
        self._current_operation.start()

# class TkApp END
######################################################

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Presents a UI that automates the generation of an Android Project and its keystore.')
    parser.add_argument('--engine', '--e', required=True,
                    help='Path to the engine root directory.')
    parser.add_argument('--project', '--p', required=True,
                    help='Path to the project root directory.')
    parser.add_argument('--build', '--b', required=True,
                    help='Path to the build directory.')
    parser.add_argument('--third_party', '--t', required=True,
                    help='Path to the 3rd Party root folder.')

    args = parser.parse_args()

    # Check if the project directory contains a json file with configuration data.
    configPath = os.path.join(args.project, DEFAULT_APG_CONFIG_FILE)
    config_data = ConfigData()
    config_data.load_from_json_file(configPath)

    # Discover the location of the keystore file if not defined yet.
    ks = config_data.keystore_settings
    if (not ks.keystore_file) or (ks.keystore_file == ""):
        ks.keystore_file = os.path.join(args.project, "app.keystore")

    # Discover the android sdk path if empty.
    if (config_data.android_sdk_path is None) or (config_data.android_sdk_path == ""):
        config_data.android_sdk_path = discovery.discover_android_sdk_path()

    config_data.engine_path = args.engine
    config_data.project_path = args.project
    config_data.build_path = args.build
    config_data.third_party_path = args.third_party

    app = TkApp(config_data, configPath)
    app.mainloop()
