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
    a = np.array([0,1,2,3,4,5,6,7,8,9])
    msgid = onesided.core.add_int_array(mb,1,a)
    assert msgid == 0
    print(mb.str())
    b = a * 10.0
    msgid = onesided.core.add_float_array(mb,1,b)
    assert msgid == 1
    print(mb.str())

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
