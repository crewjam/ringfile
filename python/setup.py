from setuptools import setup

setup(
  name="ringfile",
  description="A tool and library for writing and reading fixed size circular log files.",
  version="0.9",
  license="BSD",
  url="https://github.com/crewjam/ringfile",
  author="Ross Kinder",
  author_email="ross+czNyZXBv@kndr.org",
  py_modules=["ringfile"],
  requires=["cffi"],
  zipsafe=False
)
