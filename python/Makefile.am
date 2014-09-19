
EXTRA_DIST = \
  setup.py \
  module.h \
  module.cc \
  ringfile_test.py

CLEANFILES = \
  ringfile.egg-info \
  ringfile.so \
  ringfile_test.pyc \
  pymod-build-stamp

all-local: pymod-build-stamp

pymod-build-stamp:
	test -z "$(VPATH)" || install -m 644 "$(srcdir)/setup.py" setup.py
	VPATH=$(VPATH) $(PYTHON) setup.py build
	touch $@

clean-local:
	[ ! -d build ] || $(RM) -r build
	[ ! -d ringfile.egg-info ] || $(RM) -r ringfile.egg-info

distclean-local:
	test -z "$(VPATH)" || $(RM) module.cc ringfile.cc varint.cc setup.py ringfile_test.py

#install-exec-local: pymod-build-stamp
#	VPATH=$(VPATH) $(PYTHON) setup.py install --prefix $(DESTDIR)$(prefix)

check-local: pymod-build-stamp
	test -z "$(VPATH)" || install -m 644 "$(srcdir)/ringfile_test.py" ringfile_test.py
	VPATH=$(VPATH) $(PYTHON) setup.py test
