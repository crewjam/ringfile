
AM_CPPFLAGS = -I $(top_srcdir)/include

lib_LTLIBRARIES = libringfile.la
libringfile_la_SOURCES = \
  public_interface.cc \
  ringfile_internal.h \
  ringfile.cc \
  varint.h \
  varint.cc

bin_PROGRAMS = ringfile
ringfile_SOURCES = command.h command.cc main.cc
ringfile_LDADD = libringfile.la

TESTS = ringfile_test
check_PROGRAMS = ringfile_test

ringfile_test_SOURCES = \
  command.h \
  command.cc \
  command_test.cc \
  public_interface_test.cc \
  ringfile_test.cc \
  test_util.h \
  test_util.cc \
  varint_test.cc

ringfile_test_CPPFLAGS = $(AM_CPPFLAGS) \
  -isystem $(top_srcdir)/third_party/gtest/include \
  -I$(top_srcdir)/src \
  $(GTEST_CPPFLAGS)
ringfile_test_CXXFLAGS = $(GTEST_CXXFLAGS)
ringfile_test_LDFLAGS = $(GTEST_LDFLAGS) $(GTEST_LIBS)
ringfile_test_LDADD = \
  libringfile.la \
  ../third_party/gtest/lib/libgtest_main.la \
  ../third_party/gtest/lib/libgtest.la
