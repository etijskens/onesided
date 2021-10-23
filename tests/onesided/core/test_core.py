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
import pytest


def assert_nprocesses(nexpected):
    comm = MPI.COMM_WORLD
    assert nexpected == comm.size

def test_hello():
    assert_nprocesses(1)
    onesided.core.hello()

def test_tryout1():
    assert_nprocesses(1)
    onesided.core.tryout1()

def test_tryout2():
    assert_nprocesses(1)
    onesided.core.tryout2()

def test_tryout3():
    assert_nprocesses(1)
    onesided.core.tryout3()

def test_tryout4():
    assert_nprocesses(1)
    onesided.core.tryout4()

def test_messagebox():
    assert_nprocesses(1)
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

@pytest.mark.mpi(minsize=3)
def test_mpi_get_messages():
    comm = MPI.COMM_WORLD

    mb = onesided.core.MessageBox()
    assert mb.nMessages() == 0
    if comm.rank ==0:
        a = np.array([0,1,2,3,4,5,6,7,8,9])
        msgid = onesided.core.add_int_array(mb,1,a)
        assert msgid == 0
        print(mb.str())
        b = a * 10.0
        msgid = onesided.core.add_float_array(mb,2,b)
        assert msgid == 1

    mb.getMessages()

def test_reverse_stringstream():
    onesided.core.reverse_stringstream_2()

#===============================================================================
# The code below is for debugging a particular test in eclipse/pydev.
# (normally all tests are run with pytest)
#===============================================================================
if __name__ == "__main__":
    the_test_you_want_to_debug = test_reverse_stringstream

    print(f"__main__ running {the_test_you_want_to_debug} ...")
    the_test_you_want_to_debug()
    print('-*# finished #*-')
#===============================================================================
