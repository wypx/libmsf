#!/usr/bin/env python

import argparse
import codecs
import difflib
import os
import subprocess
import sys

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Runs clang format on all of the source "
        "files. If --fix is specified,  and compares the output "
        "with the existing file, outputting a unifiied diff if "
        "there are any necessary changes")
    parser.add_argument("--fix", default=False,
                        action="store_true",
                        help="If specified, will re-format the source "
                        "code instead of comparing the re-formatted "
                        "output, defaults to %(default)s")

    arguments = parser.parse_args()
    git_ls_cmd = 'git ls-files | grep -E "\.(c|hpp|cc|cpp|h)$"'

    file_list = ""
    try:
        file_list = subprocess.check_output(git_ls_cmd, shell=True)
    except subprocess.CalledProcessError as grepexc:                                                                                                   
        print "No new files, git_ls_cmd returns ", grepexc.returncode, grepexc.output
        sys.exit(0)

    formatted_filenames = [f.strip() for f in file_list.split()]
        
    error = False
    if arguments.fix:
        # Print out each file on its own line, but run
        # clang format once for all of the files
        print("\n".join(map(lambda x: "Formatting {}".format(x),
                            formatted_filenames)))
        subprocess.check_call(["clang-format",
                               "-i"] + formatted_filenames)
    else:
        for filename in formatted_filenames:
            print("Checking {}".format(filename))
            with open(filename, "rb") as reader:
                # Run clang-format and capture its output
                formatted = subprocess.check_output(
                    ["clang-format",
                     filename])
                formatted = codecs.decode(formatted, "utf-8")
                # Read the original file
                original = codecs.decode(reader.read(), "utf-8")
                # Run the equivalent of diff -u
                diff = list(difflib.unified_diff(
                    original.splitlines(True),
                    formatted.splitlines(True),
                    fromfile=filename,
                    tofile="{} (after clang format)".format(
                        filename)))
                if diff:
                    # Print out the diff to stderr
                    error = True
                    sys.stderr.writelines(buff.encode('utf-8') for buff in diff)

    sys.exit(1 if error else 0)
