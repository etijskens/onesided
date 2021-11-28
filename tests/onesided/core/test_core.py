#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Tests for C++ module onesided.core.
"""

import sys
sys.path.insert(0,'.')

import numpy as np
# from mpi4py import MPI
import onesided.core
import pytest


def assert_nprocesses(nexpected):
    comm = MPI.COMM_WORLD
    assert nexpected == comm.size

def test_hello():
    # if not MPI.COMM_WORLD.size == 1:
    #     return 
    ok = onesided.core.test_hello()
    print(f"ok = {ok}")
    assert ok

def test1():
    ok = onesided.core.test1()
    print(f"ok = {ok}")
    assert ok

def test_2():
    ok = onesided.core.test2()
    print(f"ok = {ok}")
    assert ok

def test_3():
    ok = onesided.core.test3()
    print(f"ok = {ok}")
    assert ok

def test_4():
    ok = onesided.core.test4()
    print(f"ok = {ok}")
    assert ok

def test_5():
    ok = onesided.core.test5()
    print(f"ok = {ok}")
    assert ok

def test_6():
    ok = onesided.core.test6()
    print(f"ok = {ok}")
    assert ok


#===============================================================================
# The code below is for debugging a particular test in eclipse/pydev.
# (normally all tests are run with pytest)
#===============================================================================
if __name__ == "__main__":
    # print("------------------------------------------------------------")
    # the_test_you_want_to_debug = test_hello
    # print(f"__main__ running {the_test_you_want_to_debug} ...")
    # the_test_you_want_to_debug()
    print("------------------------------------------------------------")
    the_test_you_want_to_debug = test_6
    print(f"__main__ running {the_test_you_want_to_debug} ...")
    the_test_you_want_to_debug()
    print("------------------------------------------------------------")
    print('-*# finished #*-')
#===============================================================================
