
AM_CPPFLAGS = -I $(top_srcdir)/include

lib_LTLIBRARIES = libringfile.la
libringfile_la_SOURCES = ringfile.cc

bin_PROGRAMS = ringfile
ringfile_SOURCES = command.cc main.cc
ringfile_LDADD = libringfile.la

TESTS = ringfile_test
check_PROGRAMS = ringfile_test

ringfile_test_SOURCES = \
  command.cc \
  command_test.cc \
  ringfile_test.cc \
  test_util.cc
ringfile_test_LDADD = libringfile.la

ringfile_test_CPPFLAGS = $(AM_CPPFLAGS) \
  -isystem $(top_srcdir)/third_party/gtest/include \
  -I$(top_srcdir)/src \
  $(GTEST_CPPFLAGS)
ringfile_test_CXXFLAGS = $(GTEST_CXXFLAGS)
ringfile_test_LDFLAGS = $(GTEST_LDFLAGS) $(GTEST_LIBS) \
  $(top_srcdir)/third_party/gtest/lib/libgtest_main.la \
  $(top_srcdir)/third_party/gtest/lib/libgtest.la
