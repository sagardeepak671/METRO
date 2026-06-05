#!/usr/bin/env python3
"""
Testing framework for SAT-based metro planning problem.

This script runs a complete test suite by:
1. Copying test files from test directory to current directory
2. Running the complete pipeline: run1.sh -> minisat -> run2.sh -> format_checker.py
3. Moving generated files back to test directory
4. Removing intermediate files from both current directory and test directory
5. Reporting results

Usage: python3 checker.py <test_directory> [test_case]
"""

import os
import sys
import shutil
import subprocess
import time
from pathlib import Path


class TestRunner:
    def __init__(self, test_directory):
        self.test_dir = Path(test_directory)
        self.pwd = Path.cwd()
        self.results = []

        # Check if test directory exists
        if not self.test_dir.exists():
            raise FileNotFoundError(f"Test directory '{test_directory}' not found")

        # Check if required scripts exist in current directory
        required_scripts = ['run1.sh', 'run2.sh', 'format_checker.py']
        for script in required_scripts:
            if not (self.pwd / script).exists():
                raise FileNotFoundError(f"Required script '{script}' not found in current directory")

    def get_test_cases(self):
        """Find all .city files in test directory and extract basenames"""
        city_files = list(self.test_dir.glob("*.city"))
        basenames = [f.stem for f in city_files]
        return sorted(basenames)

    def run_command_with_timing(self, command, description="", timeout=300):
        """Run a shell command and return success status and timing info"""
        try:
            start_time = time.time()
            result = subprocess.run(
                command,
                shell=isinstance(command, str),
                capture_output=True,
                text=True,
                cwd=self.pwd,
                timeout=timeout
            )
            end_time = time.time()
            duration = end_time - start_time

            if result.returncode != 0:
                print(f"  {description}: FAILED (exit code {result.returncode}) - {duration:.2f}s")
                if result.stdout:
                    print(f"    stdout: {result.stdout.strip()}")
                if result.stderr:
                    print(f"    stderr: {result.stderr.strip()}")
                return False, duration, result.stdout, result.stderr
            else:
                print(f"  {description}: OK - {duration:.2f}s")
                return True, duration, result.stdout, result.stderr

        except subprocess.TimeoutExpired:
            duration = timeout
            print(f"  {description}: TIMEOUT after {timeout}s")
            return False, duration, "", "Timeout"
        except Exception as e:
            print(f"  {description}: ERROR - {e}")
            return False, 0, "", str(e)

    def run_minisat(self, satinput_file, satoutput_file, timeout=60):
        """Run MiniSat with proper handling of its exit codes"""
        try:
            start_time = time.time()
            result = subprocess.run(
                ['minisat', satinput_file, satoutput_file],
                capture_output=True,
                text=True,
                cwd=self.pwd,
                timeout=timeout
            )
            end_time = time.time()
            duration = end_time - start_time

            # MiniSat exit codes: 10=SAT, 20=UNSAT, others=error
            if result.returncode == 10:
                print(f"  minisat: OK (SAT) - {duration:.2f}s")
                return True, duration, "SAT"
            elif result.returncode == 20:
                print(f"  minisat: OK (UNSAT) - {duration:.2f}s")
                return True, duration, "UNSAT"
            else:
                # treat other return codes as failure but capture output
                print(f"  minisat: FAILED (exit code {result.returncode}) - {duration:.2f}s")
                if result.stdout:
                    print(f"    stdout: {result.stdout.strip()}")
                if result.stderr:
                    print(f"    stderr: {result.stderr.strip()}")
                return False, duration, "ERROR"

        except subprocess.TimeoutExpired:
            duration = timeout
            print(f"  minisat: TIMEOUT after {timeout}s")
            return False, duration, "TIMEOUT"
        except Exception as e:
            duration = 0
            print(f"  minisat: ERROR - {e}")
            return False, duration, "EXCEPTION"

    def copy_test_file(self, basename):
        """Copy the .city file from test directory to current directory"""
        src = self.test_dir / f"{basename}.city"
        dst = self.pwd / f"{basename}.city"

        try:
            shutil.copy2(src, dst)
            print(f"  Copy {basename}.city: OK")
            return True
        except Exception as e:
            print(f"  Copy {basename}.city: FAILED - {e}")
            return False

    def cleanup_pwd_files(self, basename):
        """Remove test files from current directory"""
        files_to_remove = [
            f"{basename}.city",
            f"{basename}.satinput",
            f"{basename}.satoutput",
            f"{basename}.metromap"
            f"{basename}.pathmap"
        ]

        removed_count = 0
        for filename in files_to_remove:
            filepath = self.pwd / filename
            if filepath.exists():
                try:
                    filepath.unlink()
                    removed_count += 1
                except Exception as e:
                    print(f"  Cleanup {filename}: FAILED - {e}")

        if removed_count > 0:
            print(f"  Cleanup (pwd): OK ({removed_count} files removed)")

    def cleanup_test_dir_files(self, basename):
        """Remove intermediate files from test directory after moving them"""
        files_to_remove = [
            f"{basename}.satinput",
            f"{basename}.satoutput",
            f"{basename}.metromap"
        ]

        removed_count = 0
        for filename in files_to_remove:
            filepath = self.test_dir / filename
            if filepath.exists():
                try:
                    filepath.unlink()
                    removed_count += 1
                except Exception as e:
                    print(f"  Cleanup (test dir) {filename}: FAILED - {e}")

        if removed_count > 0:
            print(f"  Cleanup (test dir): OK ({removed_count} files removed)")

    def move_generated_files(self, basename):
        """Move generated files from current directory to test directory"""
        generated_files = [
            f"{basename}.satinput",
            f"{basename}.satoutput",
            f"{basename}.metromap"
        ]

        moved_files = []
        for filename in generated_files:
            src = self.pwd / filename
            dst = self.test_dir / filename

            if src.exists():
                try:
                    shutil.move(str(src), str(dst))
                    moved_files.append(filename)
                except Exception as e:
                    print(f"  Move {filename}: FAILED - {e}")

        if moved_files:
            print(f"  Move files: OK ({len(moved_files)} files moved to test directory)")

        return moved_files

    def run_test_case(self, basename):
        """Run complete test pipeline for a single test case"""
        print(f"\n{'='*60}")
        print(f"Testing: {basename}")
        print(f"{'='*60}")

        test_result = {
            'basename': basename,
            'steps': {},
            'timings': {},
            'success': True,
            'generated_files': [],
            'format_checker_output': '',
            'format_checker_passed': False
        }

        # Step 1: Copy .city file to current directory
        success = self.copy_test_file(basename)
        test_result['steps']['copy'] = success
        if not success:
            test_result['success'] = False
            return test_result

        # Step 2: Run run1.sh
        success, duration, stdout, stderr = self.run_command_with_timing(['bash', 'run1.sh', basename], "run1.sh")
        test_result['steps']['run1'] = success
        test_result['timings']['run1'] = duration
        if not success:
            test_result['success'] = False

        # Step 3: Run minisat
        satinput_file = f"{basename}.satinput"
        satoutput_file = f"{basename}.satoutput"

        if (self.pwd / satinput_file).exists():
            success, duration, sat_result = self.run_minisat(satinput_file, satoutput_file)
            test_result['steps']['minisat'] = success
            test_result['timings']['minisat'] = duration
            test_result['sat_result'] = sat_result
            if not success:
                test_result['success'] = False
        else:
            print(f"  minisat: SKIPPED ({satinput_file} not found)")
            test_result['steps']['minisat'] = False
            test_result['timings']['minisat'] = 0
            test_result['sat_result'] = "MISSING_INPUT"
            test_result['success'] = False

        # Step 4: Run run2.sh
        success, duration, stdout, stderr = self.run_command_with_timing(['bash', 'run2.sh', basename], "run2.sh")
        test_result['steps']['run2'] = success
        test_result['timings']['run2'] = duration
        if not success:
            test_result['success'] = False

        # Step 5: Run format_checker.py
        success, duration, stdout, stderr = self.run_command_with_timing(['python3', 'format_checker.py', basename], "format_checker")
        test_result['steps']['format_check'] = success
        test_result['timings']['format_check'] = duration
        test_result['format_checker_output'] = stdout.strip() if stdout else ''

        # Parse format checker output to determine if test passed
        if success and stdout:
            # Check if final verdict is VALID
            lines = stdout.strip().split('\n')
            for line in lines:
                if 'FINAL VERDICT:' in line and 'VALID' in line and 'INVALID' not in line:
                    test_result['format_checker_passed'] = True
                    break
            # Also handle UNSAT case reported by the checker
            if stdout.strip() == "UNSAT":
                test_result['format_checker_passed'] = True

        print('-'*40)
        print("FORMAT CHECKER OUTPUT:")
        print(test_result['format_checker_output'])
        print('-'*40)

        if not success:
            test_result['success'] = False

        # Step 6: Move generated files to test directory
        moved_files = self.move_generated_files(basename)
        test_result['generated_files'] = moved_files

        # Step 7: Remove intermediate files from test directory (so nothing remains)
        self.cleanup_test_dir_files(basename)

        # Step 8: Cleanup remaining files in pwd
        self.cleanup_pwd_files(basename)

        return test_result

    def run_all_tests(self):
        """Run tests for all test cases"""
        test_cases = self.get_test_cases()

        if not test_cases:
            print("No test cases found (no .city files in test directory)")
            return

        print(f"Found {len(test_cases)} test cases: {', '.join(test_cases)}")

        for basename in test_cases:
            result = self.run_test_case(basename)
            self.results.append(result)

        self.print_summary()

    def print_summary(self):
        """Print test summary with timing statistics"""
        print(f"\n{'='*80}")
        print("TEST SUMMARY")
        print(f"{'='*80}")

        total_tests = len(self.results)
        passed_tests = sum(1 for r in self.results if r.get('format_checker_passed', False))
        failed_tests = total_tests - passed_tests

        print(f"Total Tests: {total_tests}")
        print(f"Format Checker Passed: {passed_tests}")
        print(f"Format Checker Failed: {failed_tests}")
        print()

        # Detailed results with timing
        print(f"{'Test Case':<12} {'Result':<10} {'run1.sh':<10} {'minisat':<10} {'run2.sh':<10} {'checker':<10} {'SAT Result':<12}")
        print("-" * 75)

        for result in self.results:
            # Determine overall result based on format checker
            status = "PASS" if result.get('format_checker_passed', False) else "FAIL"

            # Get timings with fallback
            run1_time = f"{result.get('timings', {}).get('run1', 0):.2f}s"
            minisat_time = f"{result.get('timings', {}).get('minisat', 0):.2f}s"
            run2_time = f"{result.get('timings', {}).get('run2', 0):.2f}s"
            checker_time = f"{result.get('timings', {}).get('format_check', 0):.2f}s"
            sat_result = result.get('sat_result', 'UNKNOWN')

            print(f"{result['basename']:<12} {status:<10} {run1_time:<10} {minisat_time:<10} {run2_time:<10} {checker_time:<10} {sat_result:<12}")

            # Show failed steps if any
            if not result['success']:
                failed_steps = [step for step, success in result['steps'].items() if not success]
                if failed_steps:
                    print(f"{'':>12} Failed steps: {', '.join(failed_steps)}")

        print(f"\n{'='*75}")
        print("TIMING STATISTICS:")
        print(f"{'='*75}")

        if self.results:
            # Calculate average timings
            avg_run1 = sum(r.get('timings', {}).get('run1', 0) for r in self.results) / len(self.results)
            avg_minisat = sum(r.get('timings', {}).get('minisat', 0) for r in self.results) / len(self.results)
            avg_run2 = sum(r.get('timings', {}).get('run2', 0) for r in self.results) / len(self.results)
            avg_checker = sum(r.get('timings', {}).get('format_check', 0) for r in self.results) / len(self.results)

            print(f"Average run1.sh time:      {avg_run1:.2f}s")
            print(f"Average minisat time:      {avg_minisat:.2f}s")
            print(f"Average run2.sh time:      {avg_run2:.2f}s")
            print(f"Average format_check time: {avg_checker:.2f}s")

            total_avg = avg_run1 + avg_minisat + avg_run2 + avg_checker
            print(f"Average total time per test: {total_avg:.2f}s")

        print(f"\n{'='*75}")
        if failed_tests == 0:
            print("All tests passed format validation!")
        else:
            print(f"{failed_tests} test(s) failed format validation")


def main():
    if len(sys.argv) < 2 or len(sys.argv) > 3:
        print("Usage: python3 checker.py <test_directory> [test_case]")
        print("Examples:")
        print("  python3 checker.py tests                    # Run all test cases")
        print("  python3 checker.py tests case_000          # Run specific test case")
        sys.exit(1)

    test_directory = sys.argv[1]
    specific_test = sys.argv[2] if len(sys.argv) == 3 else None

    try:
        runner = TestRunner(test_directory)

        if specific_test:
            # Run specific test case
            if specific_test not in runner.get_test_cases():
                print(f"Error: Test case '{specific_test}' not found")
                print(f"Available test cases: {', '.join(runner.get_test_cases())}")
                sys.exit(1)

            result = runner.run_test_case(specific_test)
            runner.results.append(result)
            runner.print_summary()
        else:
            # Run all test cases
            runner.run_all_tests()

    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()