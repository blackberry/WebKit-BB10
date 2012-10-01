#!/usr/bin/env python


"""BlackBerry implementations of the Port interface."""

import base64
import logging
import operator
import os
import re
import subprocess
import sys
import tempfile
import time

from webkitpy.common.memoized import memoized
from webkitpy.common.net.buildbot import BuildBot
from webkitpy.common.system.executive import ScriptError
from webkitpy.layout_tests.models.test_configuration import TestConfiguration
from webkitpy.layout_tests.port import builders, server_process, Port, Driver, DriverOutput
from webkitpy.common import find_files


_log = logging.getLogger(__name__)


class BlackBerryPort(Port):
    port_name = "blackberry"

    def __init__(self, host, port_name, **kwargs):
        Port.__init__(self, host, port_name, **kwargs)
        self._webkit_base_dir = None
        self._architecture = self.get_option('arch', "armle-v7")
        self._configuration = self.get_option('configuration', "release")
        default_time_out_seconds = 35
        self.set_option_default("time_out_ms", default_time_out_seconds * 1000)
        self._test_files = None
        self._version = self.port_name

    def default_results_directory(self):
        return self._build_path('layout-test-results')

    def baseline_search_path(self):
        search_paths = []
        search_paths.append(self.name())
        ret = map(self._webkit_baseline_path, search_paths)
        return ret

    def _build_path(self, *comps):
        if self.get_option('build_directory'):
            return self._filesystem.join(self.get_option('build_directory'), *comps)

        if self._architecture is None:
            self._architecture = self.get_option('arch')
        if self._configuration is None:
            self._configuration = self.get_option('configuration')

        return self._filesystem.join(self.webkit_base_dir(), "WebKitBuild", self._architecture, self._configuration, *comps)

    # borrowed from webkitpy.layout_tests.port.Config
    def webkit_base_dir(self):
        """Returns the absolute path to the top of the WebKit tree.

        Raises an AssertionError if the top dir can't be determined."""
        # Note: this code somewhat duplicates the code in
        # scm.find_checkout_root(). However, that code only works if the top
        # of the SCM repository also matches the top of the WebKit tree. The
        # Chromium ports, for example, only check out subdirectories like
        # Tools/Scripts, and so we still have to do additional work
        # to find the top of the tree.
        #
        # This code will also work if there is no SCM system at all.
        if not self._webkit_base_dir:
            config_module_path = self._filesystem.path_to_module(self.__module__)
            self._webkit_base_dir = config_module_path[0:config_module_path.find('Tools') - 1]
        return self._webkit_base_dir

    def _path_to_driver(self):
        return self._filesystem.join(self.webkit_base_dir(), "Tools", "DumpRenderTree", "blackberry", self.driver_name())

    def _check_driver(self):
        driver_path = self._path_to_driver()
        if not self._filesystem.exists(driver_path):
            _log.error("%s was not found at %s" % (self.driver_name(), driver_path))
            return False
        return True

    def check_build(self, needs_http):
        # check driver only for now
        if not self._check_driver():
            return False
        return True

    def driver_name(self):
        return "DumpRenderTree"

    def tests(self, paths):
        """Return the list of tests found."""
        ret = Port.tests(self, paths)
        self._test_files = ret
        return ret

    def _expectations_from_skipped_files(self):
        tests_to_skip = []
        for search_path in self._skipped_file_search_paths():
            filename = self._filesystem.join(self._webkit_baseline_path(search_path), "Skipped")
            if not self._filesystem.exists(filename):
                _log.warn("Failed to open Skipped file: %s" % filename)
                continue
            _log.debug("Using Skipped file: %s" % filename)
            skipped_file_contents = self._filesystem.read_text_file(filename)
            tests_to_skip.extend(self._tests_from_skipped_file_contents(skipped_file_contents))
        return tests_to_skip

    def _tests_from_skipped_file_contents(self, skipped_file_contents):
        tests_to_skip = []
        for line in skipped_file_contents.split('\n'):
            line = line.strip()
            if line.startswith('#') or not len(line):
                continue
            tests_to_skip.append(line)
        return tests_to_skip

    def _skipped_file_search_paths(self):
        #return set([self.port_name, self.name()])
        return set([self.port_name])

    def _skipped_layout_test_paths(self):
        # Use a set to allow duplicates
        tests_to_skip = set(self._expectations_from_skipped_files())
        tests_to_skip.update(self._tests_for_other_platforms())
        tests_to_skip.update(self._skipped_tests_for_unsupported_features())
        return tests_to_skip

    def skipped_layout_tests(self, test_list):
        skipped_paths = self._skipped_layout_test_paths()
        skipped_list = set([self.relative_test_filename(f) for f in find_files.find(self._filesystem, self.layout_tests_dir(), skipped_paths)])
        skipped_tests = test_list & skipped_list
        return skipped_tests

    def _tests_for_other_platforms(self):
        # By default we will skip any directory under LayoutTests/platform
        # that isn't in our baseline search path (this mirrors what
        # old-run-webkit-tests does in findTestsToRun()).
        # Note this returns LayoutTests/platform/*, not platform/*/*.
        entries = self._filesystem.glob(self._webkit_baseline_path('*'))
        dirs_to_skip = []
        for entry in entries:
            if self._filesystem.isdir(entry) and not entry in self.baseline_search_path():
                basename = self._filesystem.basename(entry)
                dirs_to_skip.append('platform/%s' % basename)
        return dirs_to_skip

    def test_expectations(self):
        # This allows ports to use a combination of test_expectations.txt files and Skipped lists.
        expectations = self._skipped_list_as_expectations()
        expectations_path = self.path_to_test_expectations_file()
        if self._filesystem.exists(expectations_path):
            _log.debug("Using test_expectations.txt: %s" % expectations_path)
            expectations = self._filesystem.read_text_file(expectations_path) + expectations
        return expectations

    def path_to_test_expectations_file(self):
        return self._filesystem.join(self._webkit_baseline_path(self.port_name), 'test_expectations.txt')

    def _skipped_list_as_expectations(self):
        # Each Skipped file contains a list of files
        # or directories to be skipped during the test run. The total list
        # of tests to skipped is given by the contents of the generic
        # Skipped file found in platform/X plus a version-specific file
        # found in platform/X-version. Duplicate entries are allowed.
        # This routine reads those files and turns contents into the
        # format expected by test_expectations.

        tests_to_skip = self._skipped_layout_test_paths()
        skip_lines = map(lambda test_path: "BUG_SKIPPED SKIP : %s = CRASH" % test_path, tests_to_skip)
        return "\n".join(skip_lines)

    def _skipped_tests_for_unsupported_features(self):
        """Return the directories of unsupported tests. Search for the
        symbols in the symbol_list, if found add the corresponding
        directories to the skipped directory list."""
        feature_list = self._runtime_feature_list()
        directories = self._directories_for_features()

        # if DRT feature detection not supported
        if not feature_list:
            feature_list = self._supported_symbol_list()
            directories = self._directories_for_symbols()

        if not feature_list:
            return []

        skipped_directories = [directories[feature]
                              for feature in directories.keys()
                              if feature not in feature_list]
        return reduce(operator.add, skipped_directories)

    def _directories_for_features(self):
        """Return the supported feature dictionary. The keys are the
        features and the values are the directories in lists."""
        directories_for_features = {
            "Accelerated Compositing": ["compositing"],
            "3D Rendering": ["animations/3d", "transforms/3d"],
        }
        return directories_for_features

    def _directories_for_symbols(self):
        """Return the supported feature dictionary. The keys are the
        symbols and the values are the directories in lists."""
        directories_for_symbol = {
            "MathMLElement": ["mathml"],
            "GraphicsLayer": ["compositing"],
            "WebCoreHas3DRendering": ["animations/3d", "transforms/3d"],
            "WebGLShader": ["fast/canvas/webgl", "compositing/webgl", "http/tests/canvas/webgl"],
            "parseWCSSInputProperty": ["fast/wcss"],
            "isXHTMLMPDocument": ["fast/xhtmlmp"],
            "MHTMLArchive": ["mhtml"],
        }
        return directories_for_symbol

    def _runtime_feature_list(self):
        """Return the supported features of DRT. If a port doesn't support
        this DRT switch, it has to override this method to return None"""
        #driver_path = self._path_to_driver()
        #feature_list = ' '.join(os.popen(driver_path + " --print-supported-features 2>&1").readlines())
        #if "SupportedFeatures:" in feature_list:
        #    return feature_list

        # we don't support this
        return None

    def _supported_symbol_list(self):
        """Return the supported symbols of WebCore."""
        webcore_library_path = self._path_to_webcore_library()
        if not webcore_library_path:
            return None
        symbol_list = ' '.join(os.popen("nm " + webcore_library_path).readlines())
        return symbol_list

    def _generate_all_test_configurations(self):
        configurations = []
        for b_type in self.ALL_BUILD_TYPES:
            configurations.append(TestConfiguration(version=self._version, architecture=self._architecture, build_type=b_type))
        return configurations

    def setup_test_run(self):
        self._test_files -= self.skipped_layout_tests(self._test_files)

        tmp = tempfile.NamedTemporaryFile('w', delete=False)
        if self._test_files:
            for item in self._test_files:
                tmp.write(item)
                tmp.write('\n')
        tmp.close()

        cmd = []
        cmd.append(self._path_to_driver())
        cmd.append("--dump")
        cmd.append("--timeout")
        cmd.append(str(int(self.get_option('time_out_ms')) / 1000))
        cmd.append("--layout-source")
        cmd.append('%s' % self.layout_tests_dir())
        cmd.append("--layout-destination")
        cmd.append('%s/LayoutTests' % os.environ['MOUNT_POINT'])
        cmd.append(tmp.name)
        cmd.append('-')
        subprocess.call(cmd)

    def _path_to_wdiff(self):
        return 'wdiff'

    def _path_to_apache(self):
        # The Apache binary path can vary depending on OS and distribution
        # See http://wiki.apache.org/httpd/DistrosDefaultLayout
        for path in ["/usr/sbin/httpd", "/usr/sbin/apache2", "/etc/apache2"]:
            if self._filesystem.exists(path):
                return path
        _log.error("Could not find apache. Not installed or unknown path.")
        return None

    # We pass sys_platform into this method to make it easy to unit test.
    def _apache_config_file_name_for_platform(self, sys_platform):
        if sys_platform == 'cygwin':
            return 'cygwin-httpd.conf'  # CYGWIN is the only platform to still use Apache 1.3.
        if sys_platform.startswith('linux'):
            if self._is_redhat_based():
                return 'fedora-httpd.conf'  # This is an Apache 2.x config file despite the naming.
            if self._is_debian_based():
                return 'apache2-debian-httpd.conf'
        # All platforms use apache2 except for CYGWIN (and Mac OS X Tiger and prior, which we no longer support).
        return "apache2-httpd.conf"

    # FIXME: This belongs on some platform abstraction instead of Port.
    def _is_redhat_based(self):
        return self._filesystem.exists('/etc/redhat-release')

    def _is_debian_based(self):
        return self._filesystem.exists('/etc/debian_version')

    def _path_to_apache_config_file(self):
        config_file_name = self._apache_config_file_name_for_platform(sys.platform)
        return self._filesystem.join(self.layout_tests_dir(), 'http', 'conf', config_file_name)

    def _path_to_helper(self):
        return None

    def _path_to_webcore_library(self):
        full_library = self._build_path("lib", "libwebkit.so")
        if os.path.isfile(full_library):
            return full_library
        return None

    def _path_to_image_diff(self):
        return None

    def _path_to_lighttpd(self):
        return None

    def _path_to_lighttpd_modules(self):
        return None

    def _driver_class(self):
        return BlackBerryDriver


class BlackBerryDriver(Driver):
    """BlackBerry implementation of the DumpRenderTree/WebKitTestRunner interface."""

    def __init__(self, port, worker_number, pixel_tests, no_timeout=False):
        Driver.__init__(self, port, worker_number, pixel_tests, no_timeout)
        self._pixel_tests = pixel_tests
        self._server_process = None
        self._output_dir = "%s/results/" % os.environ['MOUNT_POINT']
        self._test_name = None
        self._test_done = False
        self._deadline = 0
        self._crashed = None

    def cmd_line(self, pixel_tests, per_test_args):
        cmd = self._command_wrapper(self._port.get_option('wrapper'))
        cmd.append(self._port._path_to_driver())
        cmd.append('-')
        return cmd

    def start(self):
        server_name = self._port.driver_name()
        _log.debug("Starting launcher for worker[%d]\n" % self._worker_number)
        self._server_process = server_process.ServerProcess(self._port, server_name, self.cmd_line(self._pixel_tests, None))
        os.environ['WORKER_NUMBER'] = "%d" % self._worker_number
        self._crashed = None
        self._server_process.start()

    def poll(self):
        return self._server_process.poll()

    def stop(self):
        if self._server_process:
            self._server_process.write('#DONE\n')
            time.sleep(1)
            self._server_process.stop()
            self._server_process = None
            _log.debug("Stopping launcher for worker [%d]\n" % self._worker_number)

    def _command_from_driver_input(self, driver_input):
        uri = self.test_to_uri(driver_input.test_name)
        command = uri[7:] if uri.startswith("file:///") else uri
        return command + "\n"

    def _stdout_file(self):
        return self._output_dir + self._test_name + ".dump"

    def _stderr_file(self):
        return self._output_dir + self._test_name + ".stderr"

    def _done_file(self):
        return self._output_dir + self._test_name + ".done"

    def _crash_file(self):
        return self._output_dir + self._test_name + ".dump.crash"

    def _timeouted(self):
        return time.time() >= self._deadline

    def _wait_for_test_done(self, deadline):
        if self._test_done:
            return True
        done_file = self._done_file()
        sleep_count = 0
        while (time.time() < deadline and not os.path.isfile(done_file)):
            if sleep_count == 100:
                if self.has_crashed():
                    return False
                sleep_count = 0
                self._crashed = None
            time.sleep(0.1)
            sleep_count = sleep_count + 1
        if os.path.isfile(done_file):
            os.remove(done_file)
            self._test_done = True
            return True
        _log.error("\nERROR: Hit timeout when waiting for test '%s' to finish!\n" % self._test_name)
        return False

    def run_test(self, driver_input):
        self._test_name = driver_input.test_name
        if self._server_process == None:
            self.start()

        start_time = time.time()

        if driver_input.timeout < 40000:
            timeout_s = 40
        else:
            timeout_s = int(driver_input.timeout) / 1000.0
        deadline = time.time() + timeout_s
        self._deadline = deadline

        self._test_done = False

        self._server_process.write(driver_input.test_name + '\n')

        # Block until test done
        if self._wait_for_test_done(deadline):
            try:
                text, audio = self._read_first_block(deadline)  # First block is either text or audio
                audio = None
                image = None
                actual_image_hash = None

                stderr_file = self._stderr_file()
                fo = open(stderr_file, 'r')
                error = fo.read()
                fo.close()
                return DriverOutput(text, image, actual_image_hash, audio,
                    crash=self.has_crashed(), test_time=time.time() - start_time,
                    timeout=False, error=error)
            except IOError:
                raise TestRunInterruptedException("IOError" + sys.exc_info())
        else:
            return DriverOutput(None, None, None, None,
                crash=self.has_crashed(), test_time=time.time() - start_time,
                timeout=True, error='')

    def _read_first_block(self, deadline):
        """Reads a block from the server_process and returns (text_content, audio_content)."""
        if self.has_crashed():
            return (None, None)

        block = self._read_block(deadline)
        if block.content_type == 'audio/wav':
            return (None, block.decoded_content)
        return (block.decoded_content, None)

    def has_crashed(self):
        if self._server_process is None:
            return False
        if  self._server_process.has_crashed():
            return True
        if self._crashed is not None:
            return self._crashed
        if self._test_done:
            return False

        # DumpRenderTree process is still running, but test is not finished
        # Try to detect if launcher is still running in the remote device
        self._server_process.write("#CHECK_CRASH\n")
        deadline = time.time() + 5
        while True:
            line = self._server_process.read_stdout_line(deadline)
            if line is None:
                _log.error("ERROR: Didn't get response for #CHECK_CRASH in 5 seconds\n")
                return True
            line = line.rstrip()
            _log.debug("Read a line from stdout: %s" % line)
            if '#OK' == line:
                self._crashed = False
                return False
            if '#CRASHED' == line:
                _log.error("\nERROR: Crash detected when running test '%s'!\n" % self._test_name)
                self._crashed = True
                return True
        #return self._timeouted() and not self._test_done

    def _read_optional_image_block(self, deadline):
        """Reads a block from the server_process and returns (image, actual_image_hash)."""
        if self.has_crashed():
            return (None, None)

        block = self._read_block(deadline)
        if block.content and block.content_type == 'image/png':
            return (block.decoded_content, block.content_hash)
        return (None, block.content_hash)

    def _is_warning_or_error(self, line):
        """Check if line is a system warning or error."""
        # Warning or normal error message that is OK to skip safely
        warning_headers = ['PVR:(Warning):']
        # Error message that indicate a crash
        error_headers = []
        for header in error_headers:
            if line.startswith(header):
                _log.error("ERROR: OOPS, this line indicates a crash: %s" % line)
                self._crashed = True
                return True
        for header in warning_headers:
            if line.startswith(header):
                return True
        return False

    def _read_block(self, deadline):
        LENGTH_HEADER = 'Content-Length: '
        HASH_HEADER = 'ActualHash: '
        TYPE_HEADER = 'Content-Type: '
        ENCODING_HEADER = 'Content-Transfer-Encoding: '
        content_type = None
        encoding = None
        content_hash = None
        content_length = None

        # Content is treated as binary data even though the text output is usually UTF-8.
        content = ''
        stdout_file = self._stdout_file()
        fo = open(stdout_file, 'r')
        line = fo.readline()
        # eof = False
        # fo.readline() returns empty string("") for EOF
        while ('' != line):
            if line.startswith(TYPE_HEADER) and content_type is None:
                content_type = line.split()[1]
            elif line.startswith(ENCODING_HEADER) and encoding is None:
                encoding = line.split()[1]
            elif line.startswith(LENGTH_HEADER) and content_length is None:
                content_length = int(line[len(LENGTH_HEADER):])
                # FIXME: Technically there should probably be another blank
                # line here, but DRT doesn't write one.
                content = fo.read(content_length)
            elif line.startswith(HASH_HEADER):
                content_hash = line.split()[1]
            elif line and not self._is_warning_or_error(line):
                content += line
            line = fo.readline()
        fo.close()
        return ContentBlock(content_type, encoding, content_hash, content)


class ContentBlock(object):
    def __init__(self, content_type, encoding, content_hash, content):
        self.content_type = content_type
        self.encoding = encoding
        self.content_hash = content_hash
        self.content = content
        if self.encoding == 'base64':
            self.decoded_content = base64.b64decode(content)
        else:
            self.decoded_content = content
