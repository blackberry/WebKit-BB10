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
        self._devices = os.environ['DEVICES'].split()

    def getMountPointForWorker(self, worker_number):
        return "%s%d" % (os.environ['MOUNT_POINT'], worker_number % len(self._devices))

    def getIPAddressForWorker(self, worker_number):
        return self._devices[worker_number % len(self._devices)]

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

    def driver_name(self):
        return "DumpRenderTree"

    def driver_stop_timeout(self):
        # The default 3 seconds is not enough for us to stop the remote process on the device.
        # 5 seconds is safer.
        return 5

    def tests(self, paths):
        """Return the list of tests found."""
        ret = Port.tests(self, paths)
        self._test_files = ret
        return ret

    def _generate_all_test_configurations(self):
        configurations = []
        for b_type in self.ALL_BUILD_TYPES:
            configurations.append(TestConfiguration(version=self._version, architecture=self._architecture, build_type=b_type))
        return configurations

    def setup_test_run(self):
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
        cmd.append("--device-count")
        cmd.append('%d' % len(self._devices))
        cmd.append(tmp.name)
        cmd.append('-')
        subprocess.call(cmd)

    def _path_to_webcore_library(self):
        full_library = self._build_path("lib", "libwebkit.so")
        if os.path.isfile(full_library):
            return full_library
        return None

    def _path_to_image_diff(self):
        return self._filesystem.join(self.webkit_base_dir(), 'Tools', 'DumpRenderTree', 'blackberry', 'ImageDiff')

    def _driver_class(self):
        return BlackBerryDriver


class BlackBerryDriver(Driver):
    """BlackBerry implementation of the DumpRenderTree/WebKitTestRunner interface."""

    def __init__(self, port, worker_number, pixel_tests, no_timeout=False):
        Driver.__init__(self, port, worker_number, pixel_tests, no_timeout)
        self._pixel_tests = pixel_tests
        if not pixel_tests:
            _log.warning('Warning: Ref-tests will be skipped because pixel-tests is not enabled!')
            port.set_option_default('no_ref_tests', True)
        self._server_process = None
        self._output_dir = "%s/results/" % self._port.getMountPointForWorker(worker_number)
        self._test_name = None
        self._test_done = False
        self._deadline = 0
        self._crashed = None
        self._stdout = None
        self._debug = port.get_option('debug_launcher', False)

    def cmd_line(self, pixel_tests, per_test_args):
        cmd = self._command_wrapper(self._port.get_option('wrapper'))
        cmd.append(self._port._path_to_driver())
        if self._pixel_tests:
            cmd.append('--pixel-tests')
        if self._debug:
            cmd.append('--debug')
        cmd.append('-')
        return cmd

    def start(self):
        server_name = self._port.driver_name()
        _log.debug("Starting launcher for worker[%d]\n" % self._worker_number)
        self._server_process = server_process.ServerProcess(self._port, server_name, self.cmd_line(self._pixel_tests, None))
        os.environ['DEVICEIP'] = self._port.getIPAddressForWorker(self._worker_number)
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
        command = driver_input.test_name
        if (driver_input.should_run_pixel_test and driver_input.image_hash):
            command += '?' + driver_input.image_hash
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
        while ((self._debug or time.time() < deadline) and not os.path.isfile(done_file)):
            if not self._debug and sleep_count == 100:
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

    def run_test(self, driver_input, stop_when_done):
        self._test_name = driver_input.test_name
        if self._server_process == None:
            self.start()
            if self._debug:
                raw_input("Interaction: Paused for debugging.\n"
                          "*  Please attach your debugger to the drt-launcher process on the remote device, set breakpoints, and resume the debugee process.\n"
                          "*  press enter key when you're done:")

        start_time = time.time()

        if driver_input.timeout < 40000:
            timeout_s = 40
        else:
            timeout_s = int(driver_input.timeout) / 1000.0
        deadline = time.time() + timeout_s
        self._deadline = deadline

        self._test_done = False

        self._server_process.write(self._command_from_driver_input(driver_input))

        text = None
        audio = None
        image = None
        actual_image_hash = None
        error = ''
        timed_out = False
        # Block until test done
        if self._wait_for_test_done(deadline):
            try:
                stdout_file = self._stdout_file()
                self._stdout = open(stdout_file, 'r')
                text, audio = self._read_first_block(deadline)  # First block is either text or audio
                audio = None
                if driver_input.should_run_pixel_test:
                    _log.debug("run_test: will read optional image block")
                    image, actual_image_hash = self._read_optional_image_block(deadline)
                    _log.debug('actual_image_hash=%s' % actual_image_hash)
                else:
                    image, actual_image_has = (None, None)
                self._stdout.close()
                self._stdout = None

                stderr_file = self._stderr_file()
                _stderr = open(stderr_file, 'r')
                error = _stderr.read()
                _stderr.close()
            except IOError:
                raise TestRunInterruptedException("IOError" + sys.exc_info())
        else:
            timed_out = True

        crashed = self.has_crashed()

        if stop_when_done or crashed or timed_out:
            _log.debug("Will shutdown server process: (%r, %r, %r)" % (stop_when_done, crashed, timed_out))
            self.stop()

        return DriverOutput(text, image, actual_image_hash, audio,
            crash=crashed, test_time=time.time() - start_time,
            timeout=timed_out, error=error)

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

    def _read_block(self, deadline, wait_for_stderr_eof=False):
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
        line = self._stdout.readline()
        # #EOF indicates end of the first block
        # self._stdout.readline() returns empty string("") for EOF
        while ('' != line and '#EOF\n' != line):
            if line.startswith(TYPE_HEADER) and content_type is None:
                content_type = line.split()[1]
            elif line.startswith(ENCODING_HEADER) and encoding is None:
                encoding = line.split()[1]
            elif line.startswith(LENGTH_HEADER) and content_length is None:
                content_length = int(line[len(LENGTH_HEADER):])
                # FIXME: Technically there should probably be another blank
                # line here, but DRT doesn't write one.
                content = self._stdout.read(content_length)
            elif line.startswith(HASH_HEADER):
                content_hash = line.split()[1]
            elif line and not self._is_warning_or_error(line):
                content += line
            line = self._stdout.readline()
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
