#!/usr/bin/env python
import sys

sys.stderr.write('''
%s is not installed.
Check $RSFROOT/lib/rsfconfig.py for XINC and XLIBPATH
and reinstall if necessary.
''' % sys.argv[0])
sys.exit(1)
