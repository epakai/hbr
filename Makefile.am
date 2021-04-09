HB_INFO_SOURCES = src/handbrake/options-0.9.9.h src/handbrake/options-0.10.0.h src/handbrake/options-0.10.3.h src/handbrake/options-1.0.0.h src/handbrake/options-1.1.0.h src/handbrake/options-1.2.0.h src/handbrake/options-1.3.0.h
SUPPORT_SOURCES = src/options.c src/options.h src/keyfile.c src/keyfile.h src/build_args.c src/build_args.h src/validate.c src/validate.h
GEN_SOURCES = src/gen_hbr.c src/gen_hbr.h
COMMON_SOURCES = src/util.c src/util.h

bin_PROGRAMS = hbr hbr-gen

COMMON_CFLAGS = $(GLIB2_CFLAGS) $(GIO2_CFLAGS) $(GOBJECT2_CFLAGS)

hbr_SOURCES = src/hbr.c $(SUPPORT_SOURCES) $(HB_INFO_SOURCES) $(COMMON_SOURCES)
hbr_CFLAGS = $(COMMON_CFLAGS) $(WARN_CFLAGS)
hbr_LDADD = $(GIO2_LIBS) $(GOBJECT2_LIBS) $(GLIB2_LIBS) $(WARN_LDFLAGS)

hbr_gen_SOURCES = src/hbr-gen.c $(GEN_SOURCES) $(COMMON_SOURCES)
hbr_gen_CFLAGS = $(COMMON_CFLAGS) $(WARN_CFLAGS)
hbr_gen_LDADD = $(GIO2_LIBS) $(GOBJECT2_LIBS) $(GLIB2_LIBS) $(ARGP_LIBS) $(WARN_LDFLAGS)

dist_man_MANS = doc/hbr.1 doc/hbr-gen.1

EXTRA_DIST = LICENSE autogen.sh doc/Doxyfile.in doc/Makefile.am doc/Makefile.in tests scripts

clean-local: clean-doc clean-test
distclean-local: distclean-doc

if HAVE_DOXYGEN
doc:
	$(MAKE) -C doc
clean-doc:
	$(MAKE) -C doc clean
distclean-doc:
	$(MAKE) -C doc distclean
else
doc:
	@echo Cannot generate source documentation. Doxygen not found.
clean-doc: ;
distclean-doc: ;
endif

SRC = src/hbr.c src/hbr-gen.c $(HB_INFO_SOURCES) $(SUPPORT_SOURCES) $(GEN_SOURCES) $(COMMON_SOURCES)
SOURCES := $(addprefix $(top_srcdir)/, $(SRC))
PCH = $(addsuffix .gch, $(SOURCES))

if HAVE_CLANG
clang-analyzer:
	@- clang $(COMMON_CFLAGS) --analyze $(SOURCES)
	@- rm -f $(PCH)
	@- rm *.plist
else
clang-analyzer:
	@echo *** clang not found.
endif

CPPCHECK_INCLUDES := $(filter-out -pthread, $(COMMON_CFLAGS) )
if HAVE_CPPCHECK
cppcheck:
	@- cppcheck --quiet --language=c -DDEBUG --enable=all $(CPPCHECK_INCLUDES) $(SOURCES) 
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
	@- codespell --quiet-level=3 -I $(top_srcdir)/.codespell_ignore $(SOURCES) @MAINPAGE_MDFILE@
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

if HAVE_CRAM3
cram: export CRAM_HBR=$(abs_builddir)/hbr
# Set handbrake version explicitly otherwise tests may not be reproducible
cram: export CRAM_HBR_ARGS=--hbversion=1.3.0
cram: hbr
	cram3 -qv $(top_srcdir)/tests
clean-test:
	@- rm $(top_srcdir)/tests/*.err
else
cram:
	@echo *** cram3 not found. No tests run.
clean-test: ;
endif

test: 
	$(MAKE) cram

.PHONY: doc code-analysis clang-analyzer cppcheck vera pmccabe codespell cram
