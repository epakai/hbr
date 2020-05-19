Good Series Input
  $ "$CRAM_HBR" -d -c "$TESTDIR"/configs/empty "$TESTDIR"/valid_type/good_series.hbr
  \x1b[1m# Encoding: 1/1: A - s01e001.mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 -i '/test.iso' -o 'A - s01e001.mkv' (esc)

Good Movie Input
  $ "$CRAM_HBR" -d -c "$TESTDIR"/configs/empty "$TESTDIR"/valid_type/good_movie.hbr
  \x1b[1m# Encoding: 1/1: good_movie (2000).mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 -i '/test.iso' -o 'good_movie (2000).mkv' (esc)

Movie outfile with no year (should warn)
  $ "$CRAM_HBR" -d -c "$TESTDIR"/configs/empty "$TESTDIR"/valid_type/movie_no_year.hbr 2>&1 |sed 's@'"$TESTDIR"'@TESTDIR@'
  hbr WARNING: Year not specified: (TESTDIR/valid_type/movie_no_year.hbr) [OUTFILE_A]
  \x1b[1m# Encoding: 1/1: good_movie.mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 -i '/test.iso' -o 'good_movie.mkv' (esc)

Series outfile with no episode, season, and neither (should warn in all three cases)
  $ "$CRAM_HBR" -d -c "$TESTDIR"/configs/empty "$TESTDIR"/valid_type/bad_series.hbr 2>&1 |sed 's@'"$TESTDIR"'@TESTDIR@'
  hbr WARNING: Episode number not specified: (TESTDIR/valid_type/bad_series.hbr) [OUTFILE_A]
  hbr WARNING: Season number not specified: (TESTDIR/valid_type/bad_series.hbr) [OUTFILE_B]
  hbr WARNING: Season and episode number not specified: (TESTDIR/valid_type/bad_series.hbr) [OUTFILE_C]
  \x1b[1m# Encoding: 1/3: A - s01.mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 -i '/test.iso' -o 'A - s01.mkv' (esc)
  \x1b[1m# Encoding: 2/3: A - e001.mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 -i '/test.iso' -o 'A - e001.mkv' (esc)
  \x1b[1m# Encoding: 3/3: A.mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 -i '/test.iso' -o 'A.mkv' (esc)

Type is specified in the OUTFILE section
series with no episode, season, and neither (should warn in all three cases), then a good example
movie with no year (warning), then a good example
  $ "$CRAM_HBR" -d -c "$TESTDIR"/configs/empty "$TESTDIR"/valid_type/type_varies.hbr 2>&1 |sed 's@'"$TESTDIR"'@TESTDIR@'
  hbr WARNING: Episode number not specified: (TESTDIR/valid_type/type_varies.hbr) [OUTFILE_A]
  hbr WARNING: Season number not specified: (TESTDIR/valid_type/type_varies.hbr) [OUTFILE_B]
  hbr WARNING: Season and episode number not specified: (TESTDIR/valid_type/type_varies.hbr) [OUTFILE_C]
  hbr WARNING: Year not specified: (TESTDIR/valid_type/type_varies.hbr) [OUTFILE_E]
  \x1b[1m# Encoding: 1/6: A - s01.mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 -i '/test.iso' -o 'A - s01.mkv' (esc)
  \x1b[1m# Encoding: 2/6: A - e001.mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 -i '/test.iso' -o 'A - e001.mkv' (esc)
  \x1b[1m# Encoding: 3/6: A.mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 -i '/test.iso' -o 'A.mkv' (esc)
  \x1b[1m# Encoding: 4/6: A - s01e001.mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 -i '/test.iso' -o 'A - s01e001.mkv' (esc)
  \x1b[1m# Encoding: 5/6: A.mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 -i '/test.iso' -o 'A.mkv' (esc)
  \x1b[1m# Encoding: 6/6: A (2000).mkv (esc)
  \x1b[0mHandBrakeCLI --title=1 -i '/test.iso' -o 'A (2000).mkv' (esc)
