Good readable path (input_basedir=/)
  $ "$CRAM_HBR" -d -c "$TESTDIR"/configs/empty "$TESTDIR"/valid_readable_path/good.hbr 2>&1 | sed 's@'"$TESTDIR"'@TESTDIR@g'
  \x1b[1m# Encoding: 1/1: good (2000).mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 -i '/test.iso' -o 'good (2000).mkv' (esc)

Bad readable path (input_basedir=)
  $ "$CRAM_HBR" -d -c "$TESTDIR"/configs/empty "$TESTDIR"/valid_readable_path/bad.hbr 2>&1 | sed 's@'"$TESTDIR"'@TESTDIR@g'
  hbr   ERROR: Could not read path specified by key: (TESTDIR/valid_readable_path/bad.hbr) [CONFIG] input_basedir=
  hbr   ERROR: Could not complete input file: (TESTDIR/valid_readable_path/bad.hbr)
