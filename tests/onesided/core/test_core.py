#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Tests for C++ module onesided.core.
"""

import sys
sys.path.insert(0,'.')

import numpy as np
from mpi4py import MPI
import onesided.core


def test_hello():
    onesided.core.hello()

def test_tryout1():
    onesided.core.tryout1()

def test_tryout2():
    onesided.core.tryout2()

def test_tryout3():
    onesided.core.tryout3()

def test_tryout4():
    onesided.core.tryout4()

def test_messagebox():
    mb = onesided.core.MessageBox()
    assert mb.nMessages() == 0

#===============================================================================
# The code below is for debugging a particular test in eclipse/pydev.
# (normally all tests are run with pytest)
#===============================================================================
if __name__ == "__main__":
    the_test_you_want_to_debug = test_messagebox

    print(f"__main__ running {the_test_you_want_to_debug} ...")
    the_test_you_want_to_debug()
    print('-*# finished #*-')
#===============================================================================
