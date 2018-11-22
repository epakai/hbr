SUBDIRS = src
EXTRA_DIST = autogen.sh
distclean-local:
	-rm -rf doc src

doc:
	$(MAKE) -C doc

clean-local:
	$(MAKE) -C doc clean

.PHONY: doc
