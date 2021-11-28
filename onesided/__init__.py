# -*- coding: utf-8 -*-

"""
Package onesided
=======================================

Top-level package for onesided.
"""

__version__ = "0.6.0"

try:
    import onesided.core
except ModuleNotFoundError as e:
    # Try to build this binary extension:
    from pathlib import Path
    import click
    from et_micc2.project import auto_build_binary_extension
    msg = auto_build_binary_extension(Path(__file__).parent, 'core')
    if not msg:
        import onesided.core
    else:
        click.secho(msg, fg='bright_red')


def hello(who='world'):
    """'Hello world' method.

    :param str who: whom to say hello to
    :returns: a string
    """
    result = "Hello " + who
    return result

# Your code here...