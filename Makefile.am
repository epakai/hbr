SUBDIRS = src
EXTRA_DIST = autogen.sh
distclean-local:
	-rm -rf doc src

doc:
	$(MAKE) -C doc

if HAVE_DOXYGEN
clean-local:
	$(MAKE) -C doc clean
endif

.PHONY: doc
