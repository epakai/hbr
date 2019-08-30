#!/usr/bin/env python3

"""Convert HandBrakeCLI scans to hbr sections, one per title.

This should be handy for movies with special features, but
will not be as effective where multiple episodes or shorts
share the same title, but have different chapters.

hbscan.py can be given a one or more paths for a dvd/bluray

    hbscan.py /path/to/DVD.iso /path/to/BLURAY /dev/sr0

or the output of HandBrake's scan can be piped to hbscan.py
using the '-' argument.

    HandBrakeCLI --scan -t 0 -i MOVIE.iso 2>&1 | hbscan.py -
"""

import argparse
import sys
import subprocess
import os
import pyparsing as pp


def parse_args(args=sys.argv[1:]):
    """Parse arguments."""
    parser = argparse.ArgumentParser(
        description=sys.modules[__name__].__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument('path', metavar='PATH', nargs='*',
                        help='file or directory to be scanned by HandBrake')
    parser.add_argument('--minlength', metavar='SECONDS', default=0, type=int,
                        help="Excludes titles shorter than SECONDS")
    parser.add_argument("--setiso", action='store_true', dest='setiso',
                        help='include iso_filename in outfile sections')
    parser.add_argument("-", action='store_true', dest='read_stdin',
                        help='read from stdin')

    return parser.parse_args(args)


class Chapter(object):
    def __init__(self, chapter_number, duration):
        self.chapter_number = chapter_number
        self.duration = duration


class Audio(object):
    def __init__(self, audio_number, codec, channels, iso639, frequency,
                 bitrate):
        self.audio_number = audio_number
        self.codec = codec
        self.channels = channels
        self.iso639 = iso639
        self.frequency = frequency
        self.bitrate = bitrate


class Subtitle(object):
    def __init__(self, subtitle_number, language, sub_format):
        self.subtitle_number = subtitle_number
        self.language = language
        self.sub_format = sub_format


class Title(object):
    def __init__(self, title_number):
        self.filename = None
        self.title_number = title_number
        self.playlist = None
        self.duration = None
        self.resolution = None
        self.pixel_aspect = None
        self.display_aspect = None
        self.fps = None
        self.autocrop = None
        self.combing = False
        self.detected_main = False
        self.chapters = []
        self.audio = []
        self.subtitle = []


nothing_count = 0


def donothing(s, l, t):
    global nothing_count
    nothing_count += 1
    print('Encountered unknown line in HandBrake\'s scan output:',
          file=sys.stderr)
    print(s, file=sys.stderr)


def update_path(string, loc, t):
    global current_path
    current_path = t.path


def new_title(title_list):
    def parseAction(string, loc, t):
        title_list.append(Title(t.title_num))
    return parseAction


def set_playlist(title_list):
    def parseAction(string, loc, t):
        title_list[-1].playlist = t.playlist_name
    return parseAction


def set_duration(title_list):
    def parseAction(string, loc, t):
        title_list[-1].duration = t.title_duration
    return parseAction


def set_size(title_list):
    def parseAction(string, loc, t):
        title_list[-1].resolution = t.resolution
        title_list[-1].pixel_aspect = t.pixel_aspect
        title_list[-1].display_aspect = t.display_aspect
        title_list[-1].fps = t.fps
    return parseAction


def set_autocrop(title_list):
    def parseAction(string, loc, t):
        title_list[-1].autocrop = t.autocrop.replace('/', ':')
    return parseAction


def add_chapter(title_list):
    def parseAction(string, loc, t):
        title_list[-1].chapters.append(
                Chapter(t.chapter_number, t.chapter_duration))
    return parseAction


def add_audio(title_list):
    def parseAction(string, loc, t):
        title_list[-1].audio.append(
                Audio(t.audio_number, t.audio_codec, t.audio_channels,
                      t.audio_iso639, t.audio_freq, t.audio_bitrate))
    return parseAction


def add_subtitle(title_list):
    def parseAction(string, loc, t):
        title_list[-1].subtitle.append(Subtitle(t.sub_number, t.sub_lang,
                                                t.sub_format))
    return parseAction


def combing_found(title_list):
    def parseAction(string, loc, t):
        title_list[-1].combing = True
    return parseAction


def detected_main(title_list):
    def parseAction(string, loc, t):
        title_list[-1].detected_main = True
    return parseAction


def buildParser(title_list):
    pp.ParserElement.setDefaultWhitespaceChars(' \t\r,')
    '''
   This is a really slow way to grab a list of all unicode characters.
   PyParsing 2.3.0 adds pyparsing_unicode.printables so use that instead
   '''
    pp_unicode_support = False
    if int(pp.__version__.split('.')[0]) >= 2:
        if int(pp.__version__.split('.')[1]) >= 3:
            pp_unicode_support = True
    if pp_unicode_support is False:
        unicodePrintables = ''.join(chr(c) for c in range(sys.maxunicode)
                                    if not chr(c).isspace())
    else:
        unicodePrintables = pp.pyparsing_unicode.printables

    # ignored stuff
    time = (pp.Combine(pp.Word(pp.nums, exact=2)
            + ':' + pp.Word(pp.nums, exact=2)
            + ':' + pp.Word(pp.nums, exact=2)))
    log_output = ('[' + time("time") + ']' +
                  pp.OneOrMore(pp.Word(pp.printables)))
    progress_update = "Scanning title" + pp.Word(pp.nums) + pp.OneOrMore(
            pp.Word(pp.printables))
    general_output = pp.Word(pp.printables)
    title_headers = '+' + pp.OneOrMore(pp.Word(pp.alphas)) + ':'

    EOL = pp.LineEnd().suppress()
    empty = EOL

    # title relevant matches
    title_info = pp.ZeroOrMore(' ') + '+' + pp.Word(pp.printables)
    title_info.setParseAction(donothing)

    title_path = ('[' + time + ']'
                  + 'hb_scan:'
                  + pp.Combine('path=' + pp.SkipTo(', ')("path"))
                  + pp.Combine('title_index=' + pp.Word(pp.nums)))
    title_path.setParseAction(update_path)

    title_number = '+ title' + pp.Word(pp.nums)("title_num") + ':'
    title_number.setParseAction(new_title(title_list))

    playlist = '+ playlist:' + pp.Word(pp.printables)("playlist_name")
    playlist.setParseAction(set_playlist(title_list))

    duration = '+ duration:' + time("title_duration")
    duration.setParseAction(set_duration(title_list))

    sizes = ('+ size:' + pp.Combine(pp.Word(pp.nums) + 'x'
             + pp.Word(pp.nums))("resolution")
             + 'pixel aspect:' + pp.Word(pp.nums + '/')("pixel_aspect")
             + 'display aspect:' + pp.Word(pp.nums + '.')("display_aspect")
             + pp.Word(pp.nums + '.')("fps") + 'fps')
    sizes.setParseAction(set_size(title_list))

    autocrop = '+ autocrop:' + pp.Word(pp.nums + '/')("autocrop")
    autocrop.setParseAction(set_autocrop(title_list))

    chapter = ('+'
               + pp.Word(pp.nums)("chapter_number")
               + ':'
               + pp.Word(pp.printables) * 5
               + time("chapter_duration"))
    chapter.setParseAction(add_chapter(title_list))

    audio = ('+'
             + pp.Word(pp.nums)("audio_number")
             + pp.OneOrMore(pp.Word(unicodePrintables,
                            excludeChars='()'))("audio_lang")
             + pp.Combine('(' + pp.Word(pp.alphanums + '- ')
                          ("audio_codec") + ')')
             + pp.Optional('(' + pp.OneOrMore(pp.Word(pp.alphanums + "'"))
                           + ')')
             + '(' + pp.Word(pp.nums + '.')("audio_channels") + 'ch)'
             + pp.Optional('(' + pp.OneOrMore(pp.Word(pp.alphas)) + ')')
             + '(iso639-2:' + pp.Word(pp.alphas)("audio_iso639") + ')'
             + pp.Optional(pp.Combine(pp.Word(pp.nums) + 'Hz')("audio_freq"))
             + pp.Optional(pp.Combine(pp.Word(pp.nums) + 'bps')
                           ("audio_bitrate")))
    audio.setParseAction(add_audio(title_list))

    subtitle = ('+'
                + pp.Word(pp.nums)("sub_number")
                + pp.Combine(pp.OneOrMore(
                             pp.Word(unicodePrintables,
                                     excludeChars=',([')))("sub_lang")
                + pp.ZeroOrMore(pp.Word(pp.alphanums + '(:)')) + '['
                + pp.Word(pp.alphanums)("sub_format")
                + ']')
    subtitle.setParseAction(add_subtitle(title_list))

    combing = '+ combing detected' + pp.OneOrMore(pp.Word(pp.alphas))
    combing.setParseAction(combing_found(title_list))

    main_feature = pp.Literal('+ Main Feature')
    main_feature.setParseAction(detected_main(title_list))

    line = (title_path | log_output | progress_update | main_feature |
            combing | subtitle | audio | chapter | autocrop | playlist |
            duration | sizes | title_number | title_headers | title_info |
            general_output | empty)

    return pp.OneOrMore(line)


def emit_outfile_sections(title_list):
    global args
    outfile_counter = 1
    ftr = [3600, 60, 1]
    last_title = None

    for title in title_list:
        # Don't output for titles shorter than minlength
        seconds = sum([a*b for a, b in
                      zip(ftr, map(int, title.duration.split(':')))])
        if (args.minlength > 0 and seconds < args.minlength):
            continue

        if title.filename != last_title:
            print()
            print('###', os.path.basename(os.path.normpath(title.filename)),
                  '###')
            last_title = title.filename

        print('')
        print('[OUTFILE', outfile_counter, ']', sep='')
        if args.setiso:
            print("iso_filename=", os.path.basename(os.path.normpath(
                  title.filename)), sep='')
        print('specific_name=')
        if title.playlist is None:
            print('#', title.duration)
        elif title.filename is None:
            print('#', title.playlist.lower(), title.duration)
        else:
            mpls_file_path = os.path.join(title.filename, "BDMV", "PLAYLIST",
                                          title.playlist.lower())
            mpls_dump_out = subprocess.run(['mpls_dump', '-l', mpls_file_path],
                                           stdout=subprocess.PIPE,
                                           cwd=os.getcwd(), text=True,
                                           universal_newlines=True)

            m2ts_list = []
            for line in mpls_dump_out.stdout.splitlines():
                if 'm2ts' in line:
                    m2ts_list.append(line.strip().lower())
            print('#', title.playlist.lower(), title.duration, '(',
                  ', '.join(m2ts_list), ')')
        print('title=', title.title_number, sep='')
        print('chapters=', title.chapters[0].chapter_number, '-',
              title.chapters[-1].chapter_number, sep='')
        if title.audio:
            print('# ', ','.join([str(audio.iso639 + ' (' + audio.codec + ','
                  + audio.channels + ')') for audio in title.audio]), sep='')
            print('audio=', ','.join([audio.audio_number
                  for audio in title.audio]), sep='')
        if title.subtitle:
            print('# ', ','.join([str(subtitle.language + ' [' +
                  subtitle.sub_format + ']') for subtitle in title.subtitle]),
                  sep='')
            print('subtitle=', ','.join([subtitle.subtitle_number
                  for subtitle in title.subtitle]), sep='')
        if title.detected_main:
            print('crop=0:0:0:0')
        else:
            print('crop=true')
        print('extra=')
        outfile_counter += 1


def main():
    global args
    args = parse_args()

    title_list = []
    parser = buildParser(title_list)

    if (args.read_stdin):
        global current_path
        current_path = None
        for line in sys.stdin.readlines():
            parser.parseString(line)
            if (len(title_list) > 0
                    and title_list[-1].filename is None and
                    current_path is not None):
                title_list[-1].filename = current_path
    elif (len(args.path) >= 1):
        for path in args.path:
            # TODO the HandBrakeCLI run could be done in parallel since it's
            # time consuming as long as the parsing is still done in order
            hb_out = subprocess.run(['HandBrakeCLI', '--scan', '-t', '0', '-i',
                                    path], stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, cwd=os.getcwd())
            # Strip libdvdnav lines because they have iso-8859-1 encodings
            # that are not valid utf-8, we don't need them for our purpose.
            output = b'\n'.join(line for line in hb_out.stdout.splitlines()
                                if not
                                line.startswith(b'libdvdnav:')).decode('utf-8')
            for line in output.splitlines():
                if (len(title_list) > 0 and title_list[-1].filename is None):
                    title_list[-1].filename = path
                parser.parseString(line)
    else:
        print("Missing input path or stdin argument.", file=sys.stderr)

    emit_outfile_sections(title_list)


if __name__ == '__main__':
    main()
