"""Minimal rules_ruby stand-in.

Protobuf's protobuf.bzl loads ruby_library for its own (unused) ruby
targets.  We never build those, so a no-op macro is enough to make the
load resolve.
"""

def ruby_library(**kwargs):
    pass