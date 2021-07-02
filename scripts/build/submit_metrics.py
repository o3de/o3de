#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import logging
import json
import socket
from datetime import datetime

SOCKET_TIMEOUT = 60
DATE_FORMAT = "%Y-%m-%dT%H:%M:%S"
FILEBEAT_PIPELINE = "filebeat"
FILEBEAT_DEFAULT_IP = "127.0.0.1"
FILEBEAT_DEFAULT_PORT = 9000

def parse_args():
    parser = argparse.ArgumentParser(
        prog="submit_metrics.py",
        description="Pushes a JSON document via Filebeat.",
        add_help=False
    )
    
    def file_arg(arg):
        try:
            with open(arg) as json_file:
                return json.load(json_file)
        except ValueError:
            raise argparse.ArgumentTypeError("Invalid json file '%s'" % arg)

    parser.add_argument("-f", "--file", default=None, type=file_arg, help="File containing JSON data to upload.")
    parser.add_argument("-i", "--index", default=None, help="Index to use when sending the data")
    parser.add_argument("-ip", "--filebeat_ip", default=FILEBEAT_DEFAULT_IP, help="IP address where filebeat service is listening")
    parser.add_argument("-port", "--filebeat_port", default=FILEBEAT_DEFAULT_PORT, help="Port where filebeat service is listening")
    return parser.parse_args()

def submit(index, payload, filebeat_ip = FILEBEAT_DEFAULT_IP, filebeat_port = FILEBEAT_DEFAULT_PORT):
    try:
        filebeat_address = filebeat_ip,  filebeat_port

        logging.debug(f"Connecting to Filebeat on '{filebeat_address[0]}:{filebeat_address[1]}'")
        fb_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        fb_socket.settimeout(SOCKET_TIMEOUT)
        fb_socket.connect(filebeat_address)

        event = {
            "index": index, 
            "timestamp": datetime.strptime(payload['timestamp'], DATE_FORMAT).strftime(DATE_FORMAT),
            "pipeline": FILEBEAT_PIPELINE,
            "payload": json.dumps(payload),
        }

        # Serialise event, add new line and encode as UTF-8 before sending to Filebeat.
        data = json.dumps(event) + "\n"
        data = data.encode()
        
        total_sent = 0

        logging.debug(f"Sending JSON data")
        while total_sent < len(data):
            try:
                sent = fb_socket.send(data[total_sent:])
            except BrokenPipeError:
                print("An exception occurred while sending data")
                fb_socket.close()
                total_sent = 0
            else:
                total_sent = total_sent + sent
        logging.debug("JSON data sent")
        fb_socket.close()
        logging.debug(f"Disconnected from Filebeat on '{filebeat_address[0]}:{filebeat_address[1]}'")
    except (ConnectionError, socket.timeout):
        logging.error("Failed to connect to Filebeat")
        return False
    return True

if __name__ == "__main__":
    # Parse CLI arguments.
    args = parse_args()
    if not args.index:
         logging.error(f"Index not specified")
         exit(1)
    
    if not submit(args.index, json.dumps(args.file), args.filebeat_ip, args.filebeat_port):
        exit(1)
