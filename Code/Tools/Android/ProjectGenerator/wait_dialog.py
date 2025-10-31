#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import tkinter as tk

class WaitDialog:
    """
    A cancelable modal dialog that can be helpful to
    notify the user that some work is being done
    in the background.
    """
    MAX_DOTS = 4
    def __init__(self, tk_parent, work_message: str, cancel_cb):
        self._tk_parent = tk_parent
        self._dialog = tk.Toplevel(tk_parent)

        # Display the dialog at the center of its parent.
        w = 340
        h = 100
        self._dialog.wait_visibility()
        x = tk_parent.winfo_x() + tk_parent.winfo_width()//2 - w//2
        y = tk_parent.winfo_y() + tk_parent.winfo_height()//2 - h//2

        self._dialog.geometry(f"{w}x{h}+{x}+{y}")
        self._dialog.title("Operation In Progress...")
        self._dialog.grab_set()  # Make the dialog modal.
        self._cancel_cb = cancel_cb
        lbl = tk.Label(self._dialog, text=work_message)
        lbl.pack()
        # This is a simple string that will be updated periodically
        # and its value will be bound to a tk Label widget.
        self._progress_string_var = tk.StringVar()
        self._progress_string_var.set("")
        self._progress_sign = 1 # Start positive
        lbl = tk.Label(self._dialog, textvariable=self._progress_string_var)
        lbl.pack()
        # The Cancel button
        button = tk.Button(self._dialog, text="Cancel", command=self._on_cancel_button)
        button.pack()

    def _on_cancel_button(self):
        self.close()
        self._cancel_cb()

    def close(self):
        """
        Call this function when the job is done. Will close
        and destroy this dialog
        """
        self._dialog.destroy()
        self._tk_parent.focus_set()

    def on_tick(self, delta_seconds: float):
        progress = self._progress_string_var.get()
        if self._progress_sign > 0:
            progress = f"{progress}*"
            if len(progress) >= self.MAX_DOTS:
                self._progress_sign = -1
        else:
            num_dots = len(progress)
            if num_dots < 1:
                self._progress_sign = 1
            else:
                progress = "*" * (num_dots - 1)
        self._progress_string_var.set(progress)

# class WaitDialog END
######################################################