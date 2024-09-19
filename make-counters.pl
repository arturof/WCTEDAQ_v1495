#!/usr/bin/perl

use v5.34;
use autodie;

use JSON::XS;

sub register {
  state $first = 1;
  if ($first) {
    $first = 0;
  } else {
    print ',';
  };
  printf "\n  { \"%s\", \"%s\", 0x%04x }", @_;
};

die $0, ': no configuration file' if @ARGV < 1;
die $0, ': no addresses file'     if @ARGV < 2;

open my $file, '<', $ARGV[0];
my $config = decode_json join '', <$file>;
close $file;

print 'Counter counters[] = {';

my %indices;
open my $addresses, '<', $ARGV[1];
while (<$addresses>) {
  next unless /^AR_(VERSION|\w+COUNTERS):/;
  my $group = $1;
  <$addresses>; # comment
  if ($group eq 'VERSION') {
    my $address = <$addresses>;
    $address =~ s/,\s*$//;
    $address =~ s/^\s+//;
    register 'Firmware', 'Firmware version', hex $address;
  } else {
    my $map;
    my $index;
    if ($group =~ /^([ABD])COUNTERS$/) {
      $map = $config->{input_signals};
      $group = '';
      $index = \$indices{input};
    } elsif ($group =~ /^LVL([12])_COUNTERS$/) {
      $map = $config->{"level_$1_logics"};
      $group = "L$1";
      $index = \$indices{"l$1"};
    } else {
      say STDERR $0, ': unknown group: ', $group;
      next;
    };
    $$index //= 0;
    for my $address (split /, /, <$addresses>) {
      next if $address =~ /^\s*$/;
      $address =~ s,^\s*,,;
      my $name;
      my $description;
      my $r = $map->{$$index};
      if (defined $r) {
        $name = $r->{short_name};
        $description = $r->{description};
      } else {
        $name = $group . $$index;
        $description = $name;
      };
      register $name, $description, hex $address;
      ++$$index;
    };
  };
};
close $addresses;

print "\n};\n";
