Writable directory in hbr file (/tmp)
  $ "$CRAM_HBR" "$CRAM_HBR_ARGS" -d -c "$TESTDIR"/configs/empty "$TESTDIR"/valid_writeable_path/good.hbr 2>&1 | sed 's@'"$TESTDIR"'@TESTDIR@g'
  \x1b[1m# Encoding: 1/1: good (2000).mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 -i '/test.iso' -o '/tmp/good (2000).mkv' (esc)

Non-writable directory in hbr file (empty key)
  $ "$CRAM_HBR" "$CRAM_HBR_ARGS" -d -c "$TESTDIR"/configs/empty "$TESTDIR"/valid_writeable_path/bad.hbr 2>&1 | sed 's@'"$TESTDIR"'@TESTDIR@g'
  hbr   ERROR: Unwriteable path specified by key: (TESTDIR/valid_writeable_path/bad.hbr) [CONFIG] output_basedir=
  hbr   ERROR: Could not complete input file: (TESTDIR/valid_writeable_path/bad.hbr)

Writable directory passed on command line (-o /tmp)
  $ "$CRAM_HBR" "$CRAM_HBR_ARGS" -d -o /tmp -c "$TESTDIR"/configs/empty "$TESTDIR"/valid_writeable_path/good.hbr 2>&1 | sed 's@'"$TESTDIR"'@TESTDIR@g'
  \x1b[1m# Encoding: 1/1: good (2000).mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 -i '/test.iso' -o '/tmp/good (2000).mkv' (esc)

Non-writable directory passed on command line (-o /)
  $ "$CRAM_HBR" "$CRAM_HBR_ARGS" -d -o / -c "$TESTDIR"/configs/empty "$TESTDIR"/valid_writeable_path/good.hbr 2>&1 | sed 's@'"$TESTDIR"'@TESTDIR@g'
  hbr   ERROR: Unwriteable path specified by key [--output] output_basedir=/
