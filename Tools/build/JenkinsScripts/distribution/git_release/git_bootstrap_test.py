#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import bin_download
import contextlib
import io
import os
import shutil
import ssl
import sys
import unittest


@contextlib.contextmanager
def no_stdout():
    save_stdout = sys.stdout
    sys.stdout = io.BytesIO()

    try:
        yield
    finally:
        sys.stdout = save_stdout


class BadSslTestCase(unittest.TestCase):
    def setUp(self):
        self.working_dir_path = os.path.join(os.path.expandvars("%TEMP%"), "_temp")
        self.download_file_name = "test_download.test"
        self.destination = os.path.join(self.working_dir_path, self.download_file_name)
        self.uncompressed_size = 1000000

        if not os.path.exists(self.destination):
            os.makedirs(self.destination)

    def tearDown(self):
        if os.path.exists(self.destination):
            shutil.rmtree(self.destination)

    def download_file(self, download_url):
        try:
            with bin_download.Downloader(20) as downloader:
                with no_stdout():
                    downloader.download_file(download_url, self.destination, self.uncompressed_size)

        except ssl.SSLError:
            raise

        except ssl.CertificateError:
            raise

        except Exception:
            print "\tFATAL ERROR: Unhandled exception encountered."
            raise

        return True

    def test_cloudfront_download(self):
        self.assertTrue(self.download_file("https://s3-us-west-2.amazonaws.com/lumberyard-download-artifacts-bucket/"
                                           "3rdParty/squish-ccr/20150601_lmbr_v1/filelist.1.0.0.common.json"))

    def test_expired(self):
        self.assertRaises(ssl.SSLError, self.download_file, "https://expired.badssl.com/")

    def test_wrong_host(self):
        self.assertRaises(ssl.CertificateError, self.download_file, "https://wrong.host.badssl.com/")

    def test_self_signed(self):
        self.assertRaises(ssl.SSLError, self.download_file, "https://self-signed.badssl.com/")

    def test_untrusted_root(self):
        self.assertRaises(ssl.SSLError, self.download_file, "https://untrusted-root.badssl.com/")

    def test_incomplete_chain(self):
        self.assertRaises(ssl.SSLError, self.download_file, "https://incomplete-chain.badssl.com/")

    def test_sha256(self):
        self.assertTrue(self.download_file("https://sha256.badssl.com/"))

    def test_1000_sans(self):
        self.assertTrue(self.download_file("https://1000-sans.badssl.com/"))

    def test_ecc256(self):
        self.assertTrue(self.download_file("https://ecc256.badssl.com/"))

    def test_ecc384(self):
        self.assertTrue(self.download_file("https://ecc384.badssl.com/"))

    def test_cbc(self):
        # cbc is supposed to be secure in TLS1_1 and TLS1_2
        self.assertTrue(self.download_file("https://cbc.badssl.com/"))

    def test_rc4_md5(self):
        self.assertRaises(ssl.SSLError, self.download_file, "https://rc4-md5.badssl.com/")

    def test_rc4(self):
        self.assertRaises(ssl.SSLError, self.download_file, "https://rc4.badssl.com/")

    def test_3des(self):
        self.assertRaises(ssl.SSLError, self.download_file, "https://3des.badssl.com/")

    def test_null(self):
        self.assertRaises(ssl.SSLError, self.download_file, "https://null.badssl.com/")

    def test_mozilla_intermediate(self):
        self.assertTrue(self.download_file("https://mozilla-intermediate.badssl.com/"))

    def test_mozilla_modern(self):
        self.assertTrue(self.download_file("https://mozilla-modern.badssl.com/"))

    def test_dh480(self):
        self.assertRaises(ssl.SSLError, self.download_file, "https://dh480.badssl.com/")

    def test_dh512(self):
        self.assertRaises(ssl.SSLError, self.download_file, "https://dh512.badssl.com/")

    def test_dh_small(self):
        self.assertRaises(ssl.SSLError, self.download_file, "https://dh-small.badssl.com/")

    def test_dh_composite(self):
        self.assertRaises(ssl.SSLError, self.download_file, "https://dh-composite.badssl.com/")

    def test_static_rsa(self):
        # Static RSA is still supported in TLS1_2 but is probably going to be removed in TLS1_3
        self.assertTrue(self.download_file("https://static-rsa.badssl.com/"))

    def test_hsts(self):
        self.assertTrue(self.download_file("https://hsts.badssl.com/"))

    def test_upgrade(self):
        self.assertTrue(self.download_file("https://upgrade.badssl.com/"))

    def test_preloaded_hsts(self):
        self.assertTrue(self.download_file("https://preloaded-hsts.badssl.com/"))

    def test_subdomain_preloaded_hsts(self):
        self.assertRaises(ssl.CertificateError, self.download_file, "https://subdomain.preloaded-hsts.badssl.com/")

    def test_https_everywhere(self):
        self.assertTrue(self.download_file("https://https-everywhere.badssl.com/"))

    def test_http(self):
        self.assertTrue(self.download_file("https://http.badssl.com/"))

    def test_spoofed_favicon(self):
        self.assertTrue(self.download_file("https://spoofed-favicon.badssl.com/"))

    def test_long_dashes(self):
        self.assertTrue(self.download_file(
            "https://long-extended-subdomain-name-containing-many-letters-and-dashes.badssl.com/"))

    def test_long_without_dashes(self):
        self.assertTrue(self.download_file(
            "https://longextendedsubdomainnamewithoutdashesinordertotestwordwrapping.badssl.com/"))

    def test_superfish(self):
        self.assertRaises(ssl.SSLError, self.download_file, "https://superfish.badssl.com/")

    def test_edellroot(self):
        self.assertRaises(ssl.SSLError, self.download_file, "https://edellroot.badssl.com/")

    def test_dsdtestprovider(self):
        self.assertRaises(ssl.SSLError, self.download_file, "https://dsdtestprovider.badssl.com/")

    # I can't successfully load get CRL to work
    # This is a new test in badssl.com is failing
    # as well since I don't think Qt has support for it.
    # "https://revoked.badssl.com/"

    # Excessive message size error
    # "https://10000-sans.badssl.com/"

    # This check is platform specific.
    # 8192-bit RSA keys were not supported in OSX between 2006 and 2015.
    # "https://rsa8192.badssl.com/"

    # We don't download web pages
    # "https://mixed-script.badssl.com/"
    # "https://very.badssl.com/"
    # mixed HTTP content in site
    # "https://mixed.badssl.com/"
    # implicit favicon redirects to HTTP
    # "https://mixed-favicon.badssl.com/"
    # "http://http-password.badssl.com/"
    # "http://http-login.badssl.com/"
    # "http://http-dynamic-login.badssl.com/"
    # "http://http-credit-card.badssl.com/"

    # We're not yet sure how to reject mozilla old SSL certs.
    # "https://mozilla-old.badssl.com/"

    # For some reason SSLv3 is used in these cases and we are blocking the use of SSLv3
    # "https://dh1024.badssl.com/"
    # "https://dh2048.badssl.com/"

    # This test is failing in Setup Assistant as well
    # "https://pinning-test.badssl.com/"


if __name__ == "__main__":
    unittest.main()
