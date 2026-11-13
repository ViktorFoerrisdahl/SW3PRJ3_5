"""Test start/stop using a fake systemctl, without changing system services."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

EXECUTABLE = str(Path(sys.argv.pop(1)).resolve())
FAKE_SYSTEMCTL = """#!/usr/bin/env python3
import os
import sys

with open(os.environ['PTP_TEST_LOG'], 'a') as log:
    log.write(' '.join(sys.argv[1:]) + '\\n')
if sys.argv[1] == "show":
    print("a" * 32)
elif sys.argv[1] == "is-active":
    print("active")
sys.exit(1 if sys.argv[1] == os.environ.get("PTP_TEST_FAIL") else 0)
"""


class PtpServiceTest(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        root = Path(directory.name)
        script = root / "systemctl"
        script.write_text(FAKE_SYSTEMCTL)
        script.chmod(0o755)
        for name, body in {
            "pmc": 'printf "portState %s\\n" "$PTP_TEST_PORT"',
            "journalctl": 'printf "%s\\n" "$PTP_TEST_JOURNAL"',
        }.items():
            script = root / name
            script.write_text("#!/bin/sh\n" + body + "\n")
            script.chmod(0o755)
        self.log = root / "commands.log"
        self.env = dict(
            os.environ,
            PATH=str(root) + os.pathsep + os.environ["PATH"],
            PTP_TEST_LOG=str(self.log),
            PTP_TEST_FAIL="",
        )

    def run_node(self, role="slave", fail=""):
        return subprocess.run(
            [EXECUTABLE, role],
            input="\n",
            env=dict(self.env, PTP_TEST_FAIL=fail),
            capture_output=True,
            text=True,
            timeout=5,
        )

    def test_roles_start_and_stop_their_services(self):
        for role in ("grandmaster", "slave"):
            with self.subTest(role=role):
                result = self.run_node(role)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn("PTP services started.", result.stdout)
                self.assertEqual(
                    self.log.read_text().splitlines()[-2:],
                    [
                        f"start atlas-ptp-{role}.service atlas-phc-{role}.service",
                        f"stop atlas-phc-{role}.service atlas-ptp-{role}.service",
                    ],
                )

    def test_start_failure_is_reported_and_cleaned_up(self):
        result = self.run_node(fail="start")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Could not start PTP services.", result.stderr)
        self.assertEqual(
            self.log.read_text().splitlines(),
            [
                "start atlas-ptp-slave.service atlas-phc-slave.service",
                "stop atlas-phc-slave.service atlas-ptp-slave.service",
            ],
        )

    def test_stop_failure_is_reported(self):
        result = self.run_node(fail="stop")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Could not stop PTP services.", result.stderr)

    def test_status_requires_current_locked_offsets_and_correct_port(self):
        locked = "CLOCK_REALTIME phc offset -100 s2\nmaster offset 50 s3"
        for journal, port, expected in (
            (locked, "SLAVE", 0),
            (locked, "LISTENING", 2),
            (locked + "\nmaster offset 50 s0", "SLAVE", 2),
            (locked + "\nmaster offset 200000 s2", "SLAVE", 2),
            ("", "SLAVE", 2),
        ):
            with self.subTest(journal=journal, port=port):
                result = subprocess.run(
                    [EXECUTABLE, "slave", "status"],
                    env=dict(self.env, PTP_TEST_JOURNAL=journal, PTP_TEST_PORT=port),
                    capture_output=True,
                    text=True,
                    timeout=5,
                )
                self.assertEqual(result.returncode, expected, result.stderr)
                if expected == 0:
                    self.assertEqual(result.stdout, "150\n")


    def test_invalid_role_does_not_start_services(self):
        result = self.run_node("invalid")
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(self.log.exists())


if __name__ == "__main__":
    unittest.main()
