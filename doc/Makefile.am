if HAVE_DOXYGEN

doxyfile.stamp: Doxyfile README.md
	$(DOXYGEN) Doxyfile
	echo Timestamp > doxyfile.stamp

$(MAINPAGE_MDFILE): $(top_srcdir)/README
	markdown $(top_srcdir)/README > $(MAINPAGE_MDFILE)

CLEANFILES = doxyfile.stamp README.md

all-local: doxyfile.stamp
clean-local:
	-rm -rf html latex rtf xml
endif
