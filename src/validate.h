/*
 * hbr - handbrake runner
 * Copyright (C) 2016 Joshua Honeycutt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _validate_h
#define _validate_h

#include <glib.h>
#include <gio/gio.h>

#include "build_args.h"

gboolean pre_validate_key_file(const gchar *infile);
gboolean post_validate_input_file(GKeyFile *input_keyfile, const gchar *infile,
        GKeyFile *config_keyfile);
gboolean post_validate_config_file(GKeyFile *keyfile, const gchar *infile);
gboolean post_validate_common(GKeyFile *keyfile, const gchar *infile,
        GKeyFile *config_keyfile);
gboolean has_required_keys(GKeyFile *input_keyfile, const gchar *infile,
        GKeyFile *config_keyfile);
gboolean has_requires(GKeyFile *input_keyfile, const gchar *infile,
        GKeyFile *config_keyfile);
gboolean unknown_keys_exist(GKeyFile *keyfile, const gchar *infile);
gboolean has_duplicate_groups(const gchar *infile);
gboolean has_duplicate_keys(const gchar *infile);

/*
 * Input validation for hbr specific options
 */
gboolean valid_type(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_readable_path(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_writable_path(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_filename_component(option_t *option, gchar *group,
        GKeyFile *config, const gchar *config_path);

// general input validation routines
gboolean valid_boolean(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_integer(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_integer_set(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_integer_list(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_integer_list_set(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_positive_integer(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_double_list(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_positive_double_list(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_string(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_string_set(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_string_list_set(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_string_list(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);

// specific input validation routines
gboolean valid_filename_exists(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_filename_exists_list(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_filename_dne(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_startstop_at(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_previews(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_audio(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_audio_encoder(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_audio_quality(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_audio_bitrate(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_audio_compression(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_video_quality(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_video_bitrate(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_video_framerate(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_crop(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_pixel_aspect(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_decomb(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_denoise(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_deblock(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_deinterlace(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_detelecine(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_iso_639(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_iso_639_list(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_native_dub(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_subtitle(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_gain(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_drc(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_mixdown(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_chapters(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_encopts(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_encoder_preset(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_encoder_tune(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_encoder_profile(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_encoder_level(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_nlmeans(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_nlmeans_tune(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_dither(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_subtitle_forced(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_subtitle_burned(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_subtitle_default(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_codeset(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_rotate(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_qsv_decoding(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_comb_detect(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_pad(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_unsharp(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_filespec(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);
gboolean valid_preset_name(option_t *option, gchar *group, GKeyFile *config,
        const gchar *config_path);

static gchar *iso_639_2[] = {
    "aar", "abk", "ace", "ach", "ada", "ady", "afa", "afh", "afr", "ain", "aka",
    "akk", "alb", "ale", "alg", "alt", "amh", "ang", "anp", "apa", "ara", "arc",
    "arg", "arm", "arn", "arp", "art", "arw", "asm", "ast", "ath", "aus", "ava",
    "ave", "awa", "aym", "aze", "bad", "bai", "bak", "bal", "bam", "ban", "baq",
    "bas", "bat", "bej", "bel", "bem", "ben", "ber", "bho", "bih", "bik", "bin",
    "bis", "bla", "bnt", "bod", "bos", "bra", "bre", "btk", "bua", "bug", "bul",
    "bur", "byn", "cad", "cai", "car", "cat", "cau", "ceb", "cel", "ces", "cha",
    "chb", "che", "chg", "chi", "chk", "chm", "chn", "cho", "chp", "chr", "chu",
    "chv", "chy", "cmc", "cnr", "cop", "cor", "cos", "cpe", "cpf", "cpp", "cre",
    "crh", "crp", "csb", "cus", "cym", "cze", "dak", "dan", "dar", "day", "del",
    "den", "deu", "dgr", "din", "div", "doi", "dra", "dsb", "dua", "dum", "dut",
    "dyu", "dzo", "efi", "egy", "eka", "ell", "elx", "eng", "enm", "epo", "est",
    "eus", "ewe", "ewo", "fan", "fao", "fas", "fat", "fij", "fil", "fin", "fiu",
    "fon", "fra", "fre", "frm", "fro", "frr", "frs", "fry", "ful", "fur", "gaa",
    "gay", "gba", "gem", "geo", "ger", "gez", "gil", "gla", "gle", "glg", "glv",
    "gmh", "goh", "gon", "gor", "got", "grb", "grc", "gre", "grn", "gsw", "guj",
    "gwi", "hai", "hat", "hau", "haw", "heb", "her", "hil", "him", "hin", "hit",
    "hmn", "hmo", "hrv", "hsb", "hun", "hup", "hye", "iba", "ibo", "ice", "ido",
    "iii", "ijo", "iku", "ile", "ilo", "ina", "inc", "ind", "ine", "inh", "ipk",
    "ira", "iro", "isl", "ita", "jav", "jbo", "jpn", "jpr", "jrb", "kaa", "kab",
    "kac", "kal", "kam", "kan", "kar", "kas", "kat", "kau", "kaw", "kaz", "kbd",
    "kha", "khi", "khm", "kho", "kik", "kin", "kir", "kmb", "kok", "kom", "kon",
    "kor", "kos", "kpe", "krc", "krl", "kro", "kru", "kua", "kum", "kur", "kut",
    "lad", "lah", "lam", "lao", "lat", "lav", "lez", "lim", "lin", "lit", "lol",
    "loz", "ltz", "lua", "lub", "lug", "lui", "lun", "luo", "lus", "mac", "mad",
    "mag", "mah", "mai", "mak", "mal", "man", "mao", "map", "mar", "mas", "may",
    "mdf", "mdr", "men", "mga", "mic", "min", "mis", "mkd", "mkh", "mlg", "mlt",
    "mnc", "mni", "mno", "moh", "mon", "mos", "mri", "msa", "mul", "mun", "mus",
    "mwl", "mwr", "mya", "myn", "myv", "nah", "nai", "nap", "nau", "nav", "nbl",
    "nde", "ndo", "nds", "nep", "new", "nia", "nic", "niu", "nld", "nno", "nob",
    "nog", "non", "nor", "nqo", "nso", "nub", "nwc", "nya", "nym", "nyn", "nyo",
    "nzi", "oci", "oji", "ori", "orm", "osa", "oss", "ota", "oto", "paa", "pag",
    "pal", "pam", "pan", "pap", "pau", "peo", "per", "phi", "phn", "pli", "pol",
    "pon", "por", "pra", "pro", "pus", "que", "raj", "rap", "rar", "roa", "roh",
    "rom", "ron", "rum", "run", "rup", "rus", "sad", "sag", "sah", "sai", "sal",
    "sam", "san", "sas", "sat", "scn", "sco", "sel", "sem", "sga", "sgn", "shn",
    "sid", "sin", "sio", "sit", "sla", "slk", "slo", "slv", "sma", "sme", "smi",
    "smj", "smn", "smo", "sms", "sna", "snd", "snk", "sog", "som", "son", "sot",
    "spa", "sqi", "srd", "srn", "srp", "srr", "ssa", "ssw", "suk", "sun", "sus",
    "sux", "swa", "swe", "syc", "syr", "tah", "tai", "tam", "tat", "tel", "tem",
    "ter", "tet", "tgk", "tgl", "tha", "tib", "tig", "tir", "tiv", "tkl", "tlh",
    "tli", "tmh", "tog", "ton", "tpi", "tsi", "tsn", "tso", "tuk", "tum", "tup",
    "tur", "tut", "tvl", "twi", "tyv", "udm", "uga", "uig", "ukr", "umb", "und",
    "urd", "uzb", "vai", "ven", "vie", "vol", "vot", "wak", "wal", "war", "was",
    "wel", "wen", "wln", "wol", "xal", "xho", "yao", "yap", "yid", "yor", "ypk",
    "zap", "zbl", "zen", "zgh", "zha", "zho", "znd", "zul", "zun", "zxx", "zza",
    NULL
};

#endif
