import shutil
import unittest
from unittest import skipIf
from fmpy import platform, supported_platforms, simulate_fmu, extract
from fmpy.util import add_remoting, download_file


@skipIf(platform != 'win64', "Remoting is only supported on Windows 64-bit")
class RemotingTest(unittest.TestCase):

    def test_remoting_cs(self):

        download_file('https://github.com/modelica/fmi-cross-check/raw/master/fmus/2.0/cs/win32/FMUSDK/2.0.4/vanDerPol/vanDerPol.fmu',
                      checksum='6a782ae3b3298081f9c620a17dedd54370622bd2bb78f42cb027243323a1b805')

        filename = 'vanDerPol.fmu'

        self.assertNotIn('win64', supported_platforms(filename))

        simulate_fmu(filename, fmi_type='CoSimulation', remote_platform='win32')

        add_remoting(filename)

        self.assertIn('win64', supported_platforms(filename))

        # extract all files, so we don't miss the binaries for the remote platform
        unzipdir = extract(filename)

        simulate_fmu(unzipdir, fmi_type='CoSimulation')

        shutil.rmtree(unzipdir, ignore_errors=True)

    def test_remoting_me(self):

        download_file('https://github.com/modelica/fmi-cross-check/raw/master/fmus/2.0/me/win32/FMUSDK/2.0.4/vanDerPol/vanDerPol.fmu',
                      checksum="1c2e40322586360d58fcffa8eb030dd9edb686ef7a0f2bad1e9d59d48504d56a")

        filename = 'vanDerPol.fmu'

        self.assertNotIn('win64', supported_platforms(filename))

        simulate_fmu(filename, fmi_type='ModelExchange', remote_platform='win32')

        add_remoting(filename)

        self.assertIn('win64', supported_platforms(filename))

        # extract all files, so we don't miss the binaries for the remote platform
        unzipdir = extract(filename)

        simulate_fmu(unzipdir, fmi_type='ModelExchange')

        shutil.rmtree(unzipdir, ignore_errors=True)
