#!/usr/bin/python3
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
#
# Copyright (C) Joshua Honeycutt, 2018

from shutil import which
from stat import S_ISDIR
import os
import subprocess
import argparse
import sys
import time
import magic

# This script calls dvdid from http://dvdid.cjkey.org.uk/
# Other deps: udisksctl, wget, xmlformat

# Check for dependencies
BAD_DEP = False
if which("udisksctl") is None:
    print("Could not find udisksctl command (install udisks2)")
    BAD_DEP = True
if which("wget") is None:
    print("Could not find wget command (install wget)")
    BAD_DEP = True
if which("xmlformat") is None:
    print("Could not find xmlformat command (install xmlformat-perl)")
    BAD_DEP = True
if which("dvdid") is None:
    print("Could not find dvdid command (install dvdid from "
          "http://dvdid.cjkey.org.uk/)")
    BAD_DEP = True
if BAD_DEP:
    sys.exit(1)

PARSER = argparse.ArgumentParser(description='Fetch DVD image metadata.')
PARSER.add_argument('path', metavar='PATH', nargs='?', default='.',
                    help='directory to search for DVD images')
PARSER.add_argument('-w', '--wait', dest='SECONDS', type=int, action='store',
                    default=20, help='time to wait between querying windows dvdid database')
ARGS = PARSER.parse_args()

# search arg directory (pwd default) for ISOs (i.e. <filename>.iso) by filetype
FILES = []
for root, subFolders, filenames in os.walk(ARGS.path):
    for filename in filenames:
        tp = magic.from_file(os.path.join(root, filename))
        if tp.find('UDF filesystem data') == 0:
            FILES.append(os.path.join(root, filename))

# loop over found ISO files
for dvdimage in FILES:
    #   loop mount iso
    proc = subprocess.Popen(['udisksctl', 'loop-setup', '-r', '-f', dvdimage],
                            shell=False, stdout=subprocess.PIPE)
    try:
        outs, errs = proc.communicate(timeout=15)
    except subprocess.TimeoutExpired:
        proc.kill()
        outs, errs = proc.communicate()
    if outs.split()[-1].decode('UTF-8').find("/dev/loop") != -1:
        loopdev = outs.split()[-1].decode('UTF-8')[0:-1]
    else:
        continue
    #   call dvdid to generate dvdid, store in <filename>.dvdid
    # TODO dvd title may have spaces, split on last space isn't appropriate
    volname = magic.from_file(dvdimage).split()[-1][1:-1]
    mount_path = os.path.join(os.path.expanduser('~/media/'),
                              volname)
    mode = os.stat(mount_path).st_mode
    # wait for auto mounter to mount the loop device
    while True:
        mode = os.stat(mount_path).st_mode
        if not S_ISDIR(mode):
            time.sleep(1)
        else:
            break

    proc = subprocess.Popen(['dvdid', mount_path], shell=False,
                            stdout=subprocess.PIPE)
    try:
        outs, errs = proc.communicate(timeout=15)
    except subprocess.TimeoutExpired:
        proc.kill()
        outs, errs = proc.communicate()
    dvdid = outs.decode('UTF-8').replace('|', '')
    print(dvdid)
    #   fetch metadata into file <filename>-metadata.xml
    metadata_filename = dvdimage.rsplit(".", 1)[0]+'-metadata.xml'
    url = ("http://metaservices.windowsmedia.com/pas_dvd_B/"
           "template/GetMDRDVDByCRC.xml?CRC=" + dvdid)
    print(url)
    subprocess.Popen(['wget', '-O', metadata_filename, url],
                     shell=False).wait()
    #   xmlformat -i <filename>-metadata.xml
    subprocess.Popen(['xmlformat', '-i', metadata_filename], shell=False,
                     stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    #   unmount and delete loop device
    subprocess.Popen(['udisksctl', 'unmount', '-b', loopdev], shell=False)
    subprocess.Popen(['udisksctl', 'loop-delete', '-b', loopdev], shell=False)
    #   wait a long time (determined by optional arg)
    time.sleep(ARGS.SECONDS)
