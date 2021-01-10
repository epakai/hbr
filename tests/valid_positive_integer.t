Good Input
  $ "$CRAM_HBR" "$CRAM_HBR_ARGS" -d -c "$TESTDIR"/configs/empty "$TESTDIR"/valid_positive_integer/good.hbr 2>&1 | sed 's@'"$TESTDIR"'@TESTDIR@g'
  \x1b[1m# Encoding: 1/3: good (2000).mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 --maxHeight=0 -i '/test.iso' -o 'good (2000).mkv' (esc)
  \x1b[1m# Encoding: 2/3: good (2000).mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 --maxHeight=1024 -i '/test.iso' -o 'good (2000).mkv' (esc)
  \x1b[1m# Encoding: 3/3: good (2000).mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 --maxHeight=10000000 -i '/test.iso' -o 'good (2000).mkv' (esc)

Bad values
  $ "$CRAM_HBR" "$CRAM_HBR_ARGS" -d -c "$TESTDIR"/configs/empty "$TESTDIR"/valid_positive_integer/bad.hbr 2>&1 | sed 's@'"$TESTDIR"'@TESTDIR@g'
  hbr   ERROR: Value should be a positive integer: (TESTDIR/valid_positive_integer/bad.hbr) [OUTFILE1] maxHeight=a
  hbr   ERROR: Value should be a positive integer: (TESTDIR/valid_positive_integer/bad.hbr) [OUTFILE2] maxHeight=0.0
  hbr   ERROR: Value should be a positive integer: (TESTDIR/valid_positive_integer/bad.hbr) [OUTFILE3] maxHeight=-20
  hbr   ERROR: Value should be a positive integer: (TESTDIR/valid_positive_integer/bad.hbr) [OUTFILE4] maxHeight=20%
  hbr   ERROR: Value should be a positive integer: (TESTDIR/valid_positive_integer/bad.hbr) [OUTFILE5] maxHeight=#1
  hbr   ERROR: Could not complete input file: (TESTDIR/valid_positive_integer/bad.hbr)
