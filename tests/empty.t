empty [CONFIG] and [OUTFILE] sections
  $ "$CRAM_HBR" "$CRAM_HBR_ARGS" -d -c "$TESTDIR"/configs/empty "$TESTDIR"/empty/empty.hbr 2>&1 |sed 's@'"$TESTDIR"'@TESTDIR@g'
  hbr   ERROR: Failed to merge two empty sections "CONFIG" and "CONFIG".
  hbr   ERROR: Could not complete input file: (TESTDIR/empty/empty.hbr)
