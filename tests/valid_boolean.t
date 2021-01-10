Good Input
  $ "$CRAM_HBR" "$CRAM_HBR_ARGS" -d -c "$TESTDIR"/configs/empty "$TESTDIR"/valid_boolean/good.hbr 2>&1 | sed 's@'"$TESTDIR"'@TESTDIR@g'
  \x1b[1m# Encoding: 1/2: good (2000).mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 --markers -i '/test.iso' -o 'good (2000).mkv' (esc)
  \x1b[1m# Encoding: 2/2: good (2000).mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 -i '/test.iso' -o 'good (2000).mkv' (esc)

Bad values
  $ "$CRAM_HBR" "$CRAM_HBR_ARGS" -d -c "$TESTDIR"/configs/empty "$TESTDIR"/valid_boolean/bad.hbr 2>&1 | sed 's@'"$TESTDIR"'@TESTDIR@g'
  hbr   ERROR: Invalid boolean value; Use 'true' or 'false': (TESTDIR/valid_boolean/bad.hbr) [OUTFILE1] markers=0
  hbr   ERROR: Invalid boolean value; Use 'true' or 'false': (TESTDIR/valid_boolean/bad.hbr) [OUTFILE2] markers=1
  hbr   ERROR: Invalid boolean value; Use 'true' or 'false': (TESTDIR/valid_boolean/bad.hbr) [OUTFILE3] markers=TRUE
  hbr   ERROR: Invalid boolean value; Use 'true' or 'false': (TESTDIR/valid_boolean/bad.hbr) [OUTFILE4] markers=FALSE
  hbr   ERROR: Could not complete input file: (TESTDIR/valid_boolean/bad.hbr)
