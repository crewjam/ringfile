
EXTRA_DIST = \
  setup.py \
  ringfile.py \
  ringfile_test.py

check-local:
	CPPFLAGS="-I$(srcdir)/../include" \
    LDFLAGS="-L../src/.libs" \
    DYLD_LIBRARY_PATH="../src/.libs" \
    LD_LIBRARAY_PATH="../src/.libs" \
    PYTHONPATH="$(srcdir)" \
    CFFI_TMPDIR=./__pycache__ \
    $(PYTHON) $(srcdir)/ringfile_test.py -v
