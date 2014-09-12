from setuptools import setup

import ringfile

setup(
  name="ringfile",
  description="A tool and library for writing and reading fixed size circular log files.",
  version="0.9",
  license="BSD",
  url="https://github.com/crewjam/ringfile",
  author="Ross Kinder",
  author_email="ross+czNyZXBv@kndr.org",
  ext_modules=[ringfile.Ringfile._ffi.verifier.get_extension()]
)
