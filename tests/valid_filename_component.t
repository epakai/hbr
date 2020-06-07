good queue-import-file path (queue-import-file=/filename)
  $ "$CRAM_HBR" -d -c "$TESTDIR"/configs/empty "$TESTDIR"/valid_filename_component/good.hbr 2>&1 | sed 's@'"$TESTDIR"'@TESTDIR@g'
  \x1b[1m# Encoding: 1/1: good (2000).mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 --queue-import-file=/filename -i '/test.iso' -o 'good (2000).mkv' (esc)

Bad values (one per ascii control character (1-10,11,12,14-31, 127)
Newline (10) and Carriage Return (13) are excluded because glib Keyfile eats those
  $ "$CRAM_HBR" -d -c "$TESTDIR"/configs/empty "$TESTDIR"/valid_filename_component/bad.hbr 2>&1 | sed 's@'"$TESTDIR"'@TESTDIR@g'
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE1] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE2] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE3] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE4] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE5] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE6] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE7] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE8] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE9] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE11] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE12] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE14] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE15] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE16] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE17] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE18] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE19] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE20] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE21] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE22] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE23] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE24] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE25] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE26] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE27] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE28] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE29] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE30] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE31] queue-import-file= 
  hbr   ERROR: Filename component contains control character: (TESTDIR/valid_filename_component/bad.hbr) [OUTFILE127] queue-import-file= 
  hbr   ERROR: Could not complete input file: (TESTDIR/valid_filename_component/bad.hbr)
