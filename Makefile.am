HB_INFO_SOURCES = src/handbrake/options-0.9.9.h src/handbrake/options-0.10.0.h src/handbrake/options-0.10.3.h src/handbrake/options-1.0.0.h src/handbrake/options-1.1.0.h
SUPPORT_SOURCES = src/keyfile.c src/keyfile.h src/build_args.c src/build_args.h src/validate.c src/validate.h src/util.c src/util.h

bin_PROGRAMS = hbr

hbr_SOURCES = src/hbr.c $(SUPPORT_SOURCES) $(HB_INFO_SOURCES)
hbr_CFLAGS = $(GLIB2_CFLAGS) $(GIO2_CFLAGS) $(GOBJECT2_CFLAGS)
hbr_LDADD = $(GIO2_LIBS) $(GOBJECT2_LIBS) $(GLIB2_LIBS)

EXTRA_DIST = autogen.sh doc/Doxyfile.in doc/Makefile.am doc/Makefile.in

if HAVE_DOXYGEN
doc:
	$(MAKE) -C doc
clean-local:
	$(MAKE) -C doc clean
distclean-local:
	$(MAKE) -C doc distclean
else
doc:
	@echo Cannot generate source documentation. Doxygen not found.
endif

SOURCES := $(addprefix $(top_srcdir)/, $(hbr_SOURCES))
PCH = $(addsuffix .gch, $(SOURCES))

if HAVE_CLANG
clang-analyzer:
	@- clang $(hbr_CFLAGS) --analyze $(SOURCES)
	@- rm -f $(PCH)
	@- rm *.plist
else
clang-analyzer:
	@echo *** clang not found.
endif

if HAVE_CPPCHECK
cppcheck:
	@- cppcheck --quiet $(SOURCES) 
else
cppcheck:
	@echo *** cppcheck not found.
endif

if HAVE_VERAPP
VERA_RULES = -R L001 -R L003 -R L004 -R L005 -R T001 -R T002 -R T003 -R T004 -R T005 -R T006 -R T007 -R T008 -R T009 -R T010 -R T013 -R T016 -R T019
vera:
	@- vera++ $(VERA_RULES) -s $(SOURCES)
else
vera:
	@echo *** vera++ not found.
endif

if HAVE_PMCCABE
pmccabe:
	@echo "Modified McCabe Cyclomatic Complexity"
	@echo "|   Traditional McCabe Cyclomatic Complexity"
	@echo "|       |    # Statements in function"
	@echo "|       |        |   First line of function"
	@echo "|       |        |       |   # lines in function"
	@echo "|       |        |       |       | "
	@- @pmccabe $(SOURCES) | sort -nr |head 
else
pmccabe:
	@echo *** pmccabe not found.
endif

if HAVE_CODESPELL
codespell:
	@- codespell --quiet-level=3 $(SOURCES) @MAINPAGE_MDFILE@
else
codespell:
	@echo *** codespell not found.
endif

# Some static analysis, code formatting checks, and spellcheck
code-analysis:
	$(MAKE) clang-analyzer
	$(MAKE) cppcheck
	$(MAKE) vera
	$(MAKE) codespell
	$(MAKE) pmccabe

test: ;

.PHONY: doc code-analysis clang-analyzer cppcheck vera pmccabe codespell
