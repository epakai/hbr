if HAVE_DOXYGEN

doxyfile.stamp: Doxyfile README.md $(top_srcdir)/src/*
	$(DOXYGEN) Doxyfile
	echo Timestamp > doxyfile.stamp

$(MAINPAGE_MDFILE): $(top_srcdir)/README
	markdown $(top_srcdir)/README > $(MAINPAGE_MDFILE)

CLEANFILES = doxyfile.stamp README.md

all-local: doxyfile.stamp
distclean-local:
	-rm Doxyfile Makefile
clean-local:
	-rm -rf html latex rtf xml
endif
