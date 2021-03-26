"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

#!/usr/bin/python
# Copyright (c) 2006,2007,2008 Mitch Garnaat http://garnaat.org/
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish, dis-
# tribute, sublicense, and/or sell copies of the Software, and to permit
# persons to whom the Software is furnished to do so, subject to the fol-
# lowing conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABIL-
# ITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
# SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
 
# multipart portions copyright Fabian Topfstedt
# https://gist.github.com/924094
 
 
import math
import mimetypes
from multiprocessing import Pool
import getopt, sys, os
 
import boto
from boto.exception import S3ResponseError
 
from boto.s3.connection import S3Connection
from filechunkio import FileChunkIO
 
import time
 
usage_string = """
SYNOPSIS
    s3put [-a/--access_key <access_key>] [-s/--secret_key <secret_key>]
          -b/--bucket <bucket_name> [-c/--callback <num_cb>]
          [-d/--debug <debug_level>] [-i/--ignore <ignore_dirs>]
          [-n/--no_op] [-p/--prefix <prefix>] [-k/--key_prefix <key_prefix>] 
          [-q/--quiet] [-g/--grant grant] [-w/--no_overwrite] [-r/--reduced] path

    Where
        access_key - Your AWS Access Key ID.  If not supplied, boto will
                     use the value of the environment variable
                     AWS_ACCESS_KEY_ID
        secret_key - Your AWS Secret Access Key.  If not supplied, boto
                     will use the value of the environment variable
                     AWS_SECRET_ACCESS_KEY
        bucket_name - The name of the S3 bucket the file(s) should be
                      copied to.
        path - A path to a directory or file that represents the items
               to be uploaded.  If the path points to an individual file,
               that file will be uploaded to the specified bucket.  If the
               path points to a directory, s3_it will recursively traverse
               the directory and upload all files to the specified bucket.
        debug_level - 0 means no debug output (default), 1 means normal
                      debug output from boto, and 2 means boto debug output
                      plus request/response output from httplib
        ignore_dirs - a comma-separated list of directory names that will
                      be ignored and not uploaded to S3.
        num_cb - The number of progress callbacks to display.  The default
                 is zero which means no callbacks.  If you supplied a value
                 of "-c 10" for example, the progress callback would be
                 called 10 times for each file transferred.
        prefix - A file path prefix that will be stripped from the full
                 path of the file when determining the key name in S3.
                 For example, if the full path of a file is:
                     /home/foo/bar/fie.baz
                 and the prefix is specified as "-p /home/foo/" the
                 resulting key name in S3 will be:
                     /bar/fie.baz
                 The prefix must end in a trailing separator and if it
                 does not then one will be added.
        key_prefix - A prefix to be added to the S3 key name, after any 
                     stripping of the file path is done based on the 
                     "-p/--prefix" option.
        reduced - Use Reduced Redundancy storage
        grant - A canned ACL policy that will be granted on each file
                transferred to S3.  The value of provided must be one
                of the "canned" ACL policies supported by S3:
                private|public-read|public-read-write|authenticated-read
        no_overwrite - No files will be overwritten on S3, if the file/key
                       exists on s3 it will be kept. This is useful for
                       resuming interrupted transfers. Note this is not a
                       sync, even if the file has been updated locally if
                       the key exists on s3 the file on s3 will not be
                       updated.

     If the -n option is provided, no files will be transferred to S3 but
     informational messages will be printed about what would happen.
"""
def usage():
    print usage_string
    sys.exit()
 
def submit_cb(bytes_so_far, total_bytes):
    print '%d bytes transferred / %d bytes total' % (bytes_so_far, total_bytes)
 
_last_cb_end = None # XXX blargh!
def init_throttle():
    global _last_cb_end
    _last_cb_end = time.time()
 
def throttle_cb(bytes_so_far, total_bytes):
    global _last_cb_end
    # print '%d bytes transferred / %d bytes total' % (bytes_so_far, total_bytes)
 
    d = time.time() - _last_cb_end
    time.sleep(1.0 - d)
    _last_cb_end = time.time()
 
def get_key_name(fullpath, prefix, key_prefix):
    key_name = fullpath[len(prefix):]
    l = key_name.split(os.sep)
    return key_prefix + '/'.join(l)
 
def _upload_part(bucketname, aws_key, aws_secret, multipart_id, part_num,
    source_path, offset, bytes, debug, cb, num_cb, amount_of_retries=10):
    if debug == 1:
        print "_upload_part(%s, %s, %s)" % (source_path, offset, bytes)
    """
    Uploads a part with retries.
    """
    def _upload(retries_left=amount_of_retries):
        try:
            if debug == 1:
                print 'Start uploading part #%d ...' % part_num
            conn = S3Connection(aws_key, aws_secret)
            conn.debug = debug
            bucket = conn.get_bucket(bucketname)
            for mp in bucket.get_all_multipart_uploads():
                if mp.id == multipart_id:
                    with FileChunkIO(source_path, 'r', offset=offset,
                        bytes=bytes) as fp:
                        mp.upload_part_from_file(fp=fp, part_num=part_num, cb=cb, num_cb=num_cb)
                    break
        except Exception, exc:
            if retries_left:
                _upload(retries_left=retries_left - 1)
            else:
                print 'Failed uploading part #%d' % part_num
                raise exc
        else:
            if debug == 1:
                print '... Uploaded part #%d' % part_num
 
    _upload()
 
def upload(bucketname, aws_key, aws_secret, source_path, keyname,
    reduced, debug, cb, num_cb,
    acl='private', headers={}, guess_mimetype=True, parallel_processes=4, throttle_kbps=None):
    """
    Parallel multipart upload.
    """
    conn = S3Connection(aws_key, aws_secret)
    conn.debug = debug
    bucket = conn.get_bucket(bucketname)
 
    if guess_mimetype:
        mtype = mimetypes.guess_type(keyname)[0] or 'application/octet-stream'
        headers.update({'Content-Type': mtype})
 
    mp = bucket.initiate_multipart_upload(keyname, headers=headers, reduced_redundancy=reduced)
 
    source_size = os.stat(source_path).st_size
    bytes_per_chunk = max(int(math.sqrt(5242880) * math.sqrt(source_size)),
        5242880)
    chunk_amount = int(math.ceil(source_size / float(bytes_per_chunk)))
 
    if parallel_processes == 0:
        print "doing serial upload"
        if throttle_kbps:
            print "throttling to %d kbps" % throttle_kbps
 
        for i in range(chunk_amount):
            offset = i * bytes_per_chunk
            remaining_bytes = source_size - offset
            bytes = min([bytes_per_chunk, remaining_bytes])
            part_num = i + 1
 
            if throttle_kbps:
                chunks = bytes / (throttle_kbps * 1024)
                print "uploading %d bytes in %d chunks" % (bytes, chunks)
                num_cb = chunks
                cb = throttle_cb
                init_throttle()
 
            _upload_part(bucketname, aws_key, aws_secret, mp.id, part_num,
                source_path, offset, bytes, debug, cb, num_cb, amount_of_retries=2)
    else:
        pool = Pool(processes=parallel_processes)
        for i in range(chunk_amount):
            offset = i * bytes_per_chunk
            remaining_bytes = source_size - offset
            bytes = min([bytes_per_chunk, remaining_bytes])
            part_num = i + 1
            pool.apply_async(_upload_part, [bucketname, aws_key, aws_secret, mp.id,
                part_num, source_path, offset, bytes, debug, cb, num_cb])
        pool.close()
        pool.join()
 
    if len(mp.get_all_parts()) == chunk_amount:
        mp.complete_upload()
        key = bucket.get_key(keyname)
        #key.set_acl(acl)
    else:
        mp.cancel_upload()
 
 
def main():
 
    # default values
    aws_access_key_id     = None
    aws_secret_access_key = None
    bucket_name = ''
    ignore_dirs = []
    total  = 0
    debug  = 0
    cb     = None
    num_cb = 0
    quiet  = False
    no_op  = False
    prefix = '/'
    key_prefix = ''
    grant  = None
    no_overwrite = False
    reduced = False
    num_workers = 4
    throttle_kbps=None
 
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'a:b:c::d:g:hi:k:np:qs:wr',
                                   ['access_key=', 'bucket=', 'callback=', 'debug=', 'help', 'grant=',
                                    'ignore=', 'key_prefix=', 'no_op', 'prefix=', 'quiet', 'secret_key=', 
                                    'no_overwrite', 'reduced', 'throttle=', 'num_workers='])
    except getopt.GetoptError, e:
        print e
        usage()
 
    # parse opts
    for o, a in opts:
        if o in ('-h', '--help'):
            usage()
        if o in ('-a', '--access_key'):
            aws_access_key_id = a
        if o in ('-b', '--bucket'):
            bucket_name = a
        if o in ('-c', '--callback'):
            num_cb = int(a)
            cb = submit_cb
        if o in ('-d', '--debug'):
            debug = int(a)
        if o in ('-g', '--grant'):
            grant = a
        if o in ('-i', '--ignore'):
            ignore_dirs = a.split(',')
        if o in ('-n', '--no_op'):
            no_op = True
        if o in ('w', '--no_overwrite'):
            no_overwrite = True
        if o in ('-p', '--prefix'):
            prefix = a
            if prefix[-1] != os.sep:
                prefix = prefix + os.sep
        if o in ('-k', '--key_prefix'):
            key_prefix = a
        if o in ('-q', '--quiet'):
            quiet = True
        if o in ('-s', '--secret_key'):
            aws_secret_access_key = a
        if o in ('-r', '--reduced'):
            reduced = True
        if o in ('--throttle'): # XXX this will interfere with cb params
            throttle_kbps = int(a)
        if o in ('--num_workers'):
            num_workers = int(a)
 
    if len(args) != 1:
        usage()
 
    path = os.path.expanduser(args[0])
    path = os.path.expandvars(path)
    path = os.path.abspath(path)
 
    if not bucket_name:
        print "bucket name is required!"
        usage()
 
    c = boto.connect_s3(aws_access_key_id=aws_access_key_id,
                        aws_secret_access_key=aws_secret_access_key)
    c.debug = debug
    b = c.get_bucket(bucket_name)
 
    # upload a directory of files recursively
    if os.path.isdir(path):
        if no_overwrite:
            if not quiet:
                print 'Getting list of existing keys to check against'
            keys = []
            for key in b.list(get_key_name(path, prefix, key_prefix)):
                keys.append(key.name)
        for root, dirs, files in os.walk(path):
            for ignore in ignore_dirs:
                if ignore in dirs:
                    dirs.remove(ignore)
            for file in files:
                fullpath = os.path.join(root, file)
                key_name = get_key_name(fullpath, prefix, key_prefix)
                copy_file = True
                if no_overwrite:
                    if key_name in keys:
                        copy_file = False
                        if not quiet:
                            print 'Skipping %s as it exists in s3' % file
 
                if copy_file:
                    if not quiet:
                        print 'Copying %s to %s/%s' % (file, bucket_name, key_name)
 
                    if not no_op:
                        if os.stat(fullpath).st_size == 0:
                            # 0-byte files don't work and also don't need multipart upload
                            k = b.new_key(key_name)
                            k.set_contents_from_filename(fullpath, cb=cb, num_cb=num_cb,
                                                         policy=grant, reduced_redundancy=reduced)
                        else:
                            upload(bucket_name, aws_access_key_id,
                                   aws_secret_access_key, fullpath, key_name,
                                   reduced, debug, cb, num_cb, grant or 'private',
                                   parallel_processes=num_workers, throttle_kbps=throttle_kbps)
                total += 1
 
    # upload a single file
    elif os.path.isfile(path):
        key_name = get_key_name(os.path.abspath(path), prefix, key_prefix)
        copy_file = True
        if no_overwrite:
            if b.get_key(key_name):
                copy_file = False
                if not quiet:
                    print 'Skipping %s as it exists in s3' % path
 
        if copy_file:
            if not quiet:
                print 'Copying %s to %s/%s' % (path, bucket_name, key_name)
 
            if not no_op:
                if os.stat(path).st_size == 0:
                    # 0-byte files don't work and also don't need multipart upload
                    k = b.new_key(key_name)
                    k.set_contents_from_filename(path, cb=cb, num_cb=num_cb, policy=grant,
                                                 reduced_redundancy=reduced)
                else:
                    upload(bucket_name, aws_access_key_id,
                           aws_secret_access_key, path, key_name,
                           reduced, debug, cb, num_cb, grant or 'private',
                           parallel_processes=num_workers, throttle_kbps=throttle_kbps)
 
if __name__ == "__main__":
    main()