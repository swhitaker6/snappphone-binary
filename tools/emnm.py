#!/usr/bin/env python3
# Copyright 2016 The Emscripten Authors.  All rights reserved.
# Emscripten is available under two separate licenses, the MIT license and the
# University of Illinois/NCSA Open Source License.  Both these licenses can be
# found in the LICENSE file.

"""Wrapper scripte around `llvm-nm`.
"""

import os
import sys

__scriptdir__ = os.path.dirname(os.path.abspath(__file__))
__rootdir__ = os.path.dirname(__scriptdir__)
sys.path.append(__rootdir__)

from tools import shared

cmd = [shared.LLVM_NM] + sys.argv[1:]
sys.exit(shared.run_process(cmd, stdin=sys.stdin, check=False).returncode)
