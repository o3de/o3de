"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Device Farm Schecule Run
"""

import argparse
import datetime
import json
import logging
import os
import subprocess
import sys
import time
import requests

logger = logging.getLogger(__name__)



def bake_template(filename, values):
    """Open a template and replace values. Return path to baked file."""
    # Open the options json template and replace with real values.
    with open(filename, 'r') as in_file:
        data = in_file.read()
        for key, value in values.iteritems():
            data = data.replace(key, str(value))
    filename_out = os.path.join('temp', filename)
    with open(filename_out, 'w') as out_file:
        out_file.write(data)
    return filename_out

def execute_aws_command(args):
    """ Execut the aws cli devicefarm command. """
    # Use .cmd on Windows, not sure exactly why, but aws will not be found without it.
    aws_executable = 'aws.cmd' if sys.platform.startswith('win') else 'aws'
    aws_args = [aws_executable, 'devicefarm', '--region', 'us-west-2'] + args
    logger.info("Running {} ...".format(" ".join(aws_args)))
    p = subprocess.Popen(aws_args, stdout=subprocess.PIPE)
    out, err = p.communicate()
    if p.returncode != 0:
        msg = "Command '{}' failed. return code: {} out: {} err: {}".format(
            " ".join(aws_args),
            p.returncode,
            out,
            err
        )
        raise Exception(msg)
    return out


def find_or_create_project(project_name):
    """ Find the project by name, or create a new one. """
    list_projects_data = json.loads(execute_aws_command(['list-projects']))
    
    # return the arn if it is found
    for project_data in list_projects_data['projects']:
        if project_data['name'] == project_name:
            logger.info("Found existing project named {}.".format(project_name))
            return project_data['arn']
            
    # project not found, create a new project with the give name
    project_data = json.loads(execute_aws_command(['create-project', '--name', project_name]))
    return project_data['project']['arn']
    
def find_or_create_device_pool(project_name, device_pool_name, device_arns):
    """ Find the device pool in the project by name, or create a new one. """
    list_device_pools_data = json.loads(execute_aws_command(['list-device-pools', '--arn', project_name]))

    # return the arn if it is found
    for device_pool_data in list_device_pools_data['devicePools']:
        if device_pool_data['name'] == device_pool_name:
            logger.info("Found existing device pool named {}.".format(device_pool_name))
            return device_pool_data['arn']

    device_pool_json_path_out = bake_template(
        'device_farm_default_device_pool_template.json',
        {'%DEVICE_ARN_LIST%' : device_arns})

    # create a default device pool
    args = [
        'create-device-pool',
        '--project-arn',
        project_name,
        '--name',
        device_pool_name,
        '--rules',
        "file://{}".format(device_pool_json_path_out)]
    device_pools_data = json.loads(execute_aws_command(args))    
    return device_pools_data['devicePool']['arn']
    
    
def create_upload(project_arn, path, type):
    """ Create an upload and return the ARN """
    args = ['create-upload', '--project-arn', project_arn, '--name', os.path.basename(path), '--type', type]
    upload_data = json.loads(execute_aws_command(args))
    return upload_data['upload']['arn'], upload_data['upload']['url']

def send_upload(filename, url):
    """ Upload a file with a put request. """
    logger.info("Sending upload {} ...".format(filename))
    with open(filename, 'rb') as uploadfile:
        data = uploadfile.read()
    headers = {"content-type": "application/octet-stream"}
    output = requests.put(url, data=data, allow_redirects=True, headers=headers)
    logger.info("Sent upload {}.".format(output))
    
def wait_for_upload_to_finish(poll_time, upload_arn):
    """ Wait for an upload to finish by polling for status """
    logger.info("Waiting for upload {} ...".format(upload_arn))
    upload_data = json.loads(execute_aws_command(['get-upload', '--arn', upload_arn]))
    while not upload_data['upload']['status'] in ['SUCCEEDED', 'FAILED']:
        time.sleep(poll_time)
        upload_data = json.loads(execute_aws_command(['get-upload', '--arn', upload_arn]))       
    
    if upload_data['upload']['status'] != 'SUCCEEDED':
        raise Exception('Upload failed.')

def upload(poll_time, project_arn, path, type):
    """ Create the upload on the Device Farm, upload the file and wait for completion. """
    arn, url = create_upload(project_arn, path, type)
    send_upload(path, url)
    wait_for_upload_to_finish(poll_time, arn)
    return arn
    
def schedule_run(project_arn, app_arn, device_pool_arn, test_spec_arn, test_bundle_arn, execution_timeout):
    """ Schecule the test run on the Device Farm """
    run_name = "LY LT {}".format(datetime.datetime.now().strftime("%I:%M%p on %B %d, %Y"))
    logger.info("Scheduling run {} ...".format(run_name))

    schedule_run_test_json_path_out = bake_template(
        'device_farm_schedule_run_test_template.json',
        {'%TEST_SPEC_ARN%' : test_spec_arn, '%TEST_PACKAGE_ARN%' : test_bundle_arn})

    execution_configuration_json_path_out = bake_template(
        'device_farm_schedule_run_execution_configuration_template.json',
        {'%EXECUTION_TIMEOUT%' : execution_timeout})

    args = [
        'schedule-run',
        '--project-arn',
        project_arn,
        '--app-arn',
        app_arn,
        '--device-pool-arn',
        device_pool_arn,
        '--name',
        "\"{}\"".format(run_name),
        '--test',
        "file://{}".format(schedule_run_test_json_path_out),
        '--execution-configuration',
        "file://{}".format(execution_configuration_json_path_out)]
    
    schedule_run_data = json.loads(execute_aws_command(args))    
    return schedule_run_data['run']['arn']     

def download_file(url, output_path):
    """ download a file from a url, save in output_path """
    try:
        r = requests.get(url, stream=True)
        r.raise_for_status()
        output_folder = os.path.dirname(output_path)
        if not os.path.exists(output_folder):
            os.makedirs(output_folder)
        with open(output_path, 'wb') as f:
            for chunk in r.iter_content(chunk_size=8192): 
                if chunk:
                    f.write(chunk)
    except requests.exceptions.RequestException as e:
        logging.exception("Failed request for downloading file from {}.".format(url))
        return False
    except IOError as e:
        logging.exception("Failed writing to file {}.".format(output_path))
        return False
    return True
    
def download_artifacts(run_arn, artifacts_output_folder):
    """ 
    Download run artifacts and write to path set in artifacts_output_folder.
    """
    logging.basicConfig(level=logging.DEBUG)

    list_jobs_data = json.loads(execute_aws_command(['list-jobs', '--arn', run_arn]))
    for job_data in list_jobs_data['jobs']:
        logger.info("Downloading artifacts for {} ...".format(job_data['name']))
        safe_job_name = "".join(x for x in job_data['name'] if x.isalnum())
        list_artifacts_data = json.loads(execute_aws_command(['list-artifacts', '--arn', job_data['arn'], '--type', 'FILE']))
        for artifact_data in list_artifacts_data['artifacts']:
            # A run may contain many jobs. Usually each job is one device type.
            # Each job has 3 stages: setup, test and shutdown. You can tell what
            # stage an artifact is from based on the ARN.
            # We only care about artifacts from the main stage of the job,
            # not the setup or tear down artifacts. So parse the ARN and look
            # for the 00001 identifier.
            print artifact_data['arn']
            if artifact_data['arn'].split('/')[3] == '00001':
                logger.info("Downloading artifacts {} ...".format(artifact_data['name']))
                output_filename = "{}.{}".format(
                    "".join(x for x in artifact_data['name'] if x.isalnum()),
                    artifact_data['extension'])
                output_path = os.path.join(artifacts_output_folder, safe_job_name, output_filename)
                if not download_file(artifact_data['url'], output_path):
                    msg = "Failed to download file from {} and save to {}".format(artifact_data['url'], output_path)
                    logger.error(msg)

def main():

    parser = argparse.ArgumentParser(description='Upload and app and schedule a run on the Device Farm.')
    parser.add_argument('--app-path', required=True, help='Path of the app file.')
    parser.add_argument('--test-spec-path', required=True, help='Path of the test spec yaml.')
    parser.add_argument('--test-bundle-path', required=True, help='Path of the test bundle zip.')
    parser.add_argument('--project-name', required=True, help='The name of the project.')
    parser.add_argument('--device-pool-name', required=True, help='The name of the device pool.')
    parser.add_argument('--device-arns',
        default='\\"arn:aws:devicefarm:us-west-2::device:6CCDF49186B64E3FB27B9346AC9FAEC1\\"',
        help='List of device ARNs. Used when existing pool is not found by name. Default is Galaxy S8.')
    parser.add_argument('--wait-for-result', default="true", help='Set to "true" to wait for result of run.')
    parser.add_argument('--download-artifacts', default="true", help='Set to "true" to download artifacts after run. requires --wait-for-result')
    parser.add_argument('--artifacts-output-folder', default="temp", help='Folder to place the downloaded artifacts.')
    parser.add_argument('--upload-poll-time', default=10, help='How long to wait between polling upload status.')
    parser.add_argument('--run-poll-time', default=60, help='How long to wait between polling run status.')
    parser.add_argument('--run-execution-timeout', default=60, help='Run execution timeout.')
    parser.add_argument('--test-names', nargs='+', help='A list of test names to run, default runs all tests.')
    

    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG)
        
    # Find the project by name, or create a new one.
    project_arn = find_or_create_project(args.project_name)
    
    # Find the device pool in the project by name, or create a new one.
    device_pool_arn = find_or_create_device_pool(project_arn, args.device_pool_name, args.device_arns)

    # Bake out EXTRA_ARGS option with args.test_names
    extra_args = ""
    if args.test_names:
        extra_args = "--test-names {}".format(" ".join("\"{}\"".format(test_name) for test_name in args.test_names))
    
    test_spec_path_out = bake_template(
        args.test_spec_path,
        {'%EXTRA_ARGS%' : extra_args})
        
    # Upload test spec and test bundle (Appium js is just a generic avenue to our own custom code).
    test_spec_arn = upload(args.upload_poll_time, project_arn, test_spec_path_out, 'APPIUM_NODE_TEST_SPEC')
    test_bundle_arn = upload(args.upload_poll_time, project_arn, args.test_bundle_path, 'APPIUM_NODE_TEST_PACKAGE')

    # Upload the app.
    type = 'ANDROID_APP' if args.app_path.lower().endswith('.apk') else 'IOS_APP'
    app_arn = upload(args.upload_poll_time, project_arn, args.app_path, type)

    # Schedule the test run.
    run_arn = schedule_run(project_arn, app_arn, device_pool_arn, test_spec_arn, test_bundle_arn, args.run_execution_timeout)
    
    logger.info('Run scheduled.')
    
    # Wait for run, exit with failure if test run fails. 
    # strcmp with true for easy of use jenkins boolean env var.
    if args.wait_for_result.lower() == 'true':
    
        # Runs can take a long time, so just poll once a mintue by default.
        run_data = json.loads(execute_aws_command(['get-run', '--arn', run_arn]))
        while run_data['run']['result'] == 'PENDING':
            logger.info("Run status: {} waiting {} seconds ...".format(run_data['run']['result'], args.run_poll_time))
            time.sleep(args.run_poll_time)
            run_data = json.loads(execute_aws_command(['get-run', '--arn', run_arn]))

        # Download run artifacts. strcmp with true for easy of use jenkins boolean env var.
        if args.download_artifacts.lower() == 'true':
            download_artifacts(run_arn, args.artifacts_output_folder)

        # If the run did not pass raise an exception to fail this jenkins job.
        if run_data['run']['result'] != 'PASSED':        
            # Dump all of the run info.
            logger.info(run_data)
            # Raise an exception to fail this test.
            msg = "Run fail with result {}\nRun ARN: {}".format(run_data['run']['result'], run_arn)
            raise Exception(msg)

        logger.info('Run passed.')
        
if __name__== "__main__":
    main()
