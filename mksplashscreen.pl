#!/usr/bin/perl
#  USBserLCD Parallel T6963C LCD to USB converter
#  Copyright (C) 2016 Manuel Reimer <manuel.reimer@gmx.de>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

use strict;
use warnings;
use File::Basename;

my $pbmpath = 'splashscreen.pbm';
my $hpath = 'splashscreen.h';

sub Usage {
  print basename($0) . " [PBM Path]\n";
  print "\n";
  print "Tool to convert PBM file to \"splashscreen.h\" for usage with USBserLCD.\n";
  print "\"splashscreen.pbm\" is used as input file, if no path is given on command line.\n";
  exit(0);
}

{
  if (@ARGV == 1) {
    Usage() if ($ARGV[0] eq '-h' || $ARGV[0] eq '--help');
    $pbmpath = $ARGV[0];
  }

  open(my $fhpbm, '<:raw', $pbmpath) or die("Failed to read $pbmpath: $!\n");
  open(my $fhh, '>', $hpath) or die("Failed to open $hpath: $!\n");

  $/ = "\x0A";
  chomp(my $header = <$fhpbm>);
  die ("Error: $pbmpath not in RAW PBM format!\n") if ($header ne 'P4');
  chomp(my $size = <$fhpbm>);
  chomp($size = <$fhpbm>) if ($size =~ /^#/);
  die ("Error: Image size has to be 240x128!\n") if ($size ne '240 128');

  print $fhh "const unsigned char SplashImage[] PROGMEM = {";
  for (my $index = 0; $index < (240*128)/8; $index++) {
    my $bytes_read = read($fhpbm, my $byte, 1);
    die("Error while reading image data!\n") if ($bytes_read != 1);
    print  $fhh ', ' if ($index > 0);
    printf $fhh '0x%02X', ord($byte);
  }
  print $fhh "};\n";
}
