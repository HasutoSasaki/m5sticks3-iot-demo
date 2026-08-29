#!/bin/zsh
set -euo pipefail
umask 077

if (( $# != 1 )); then
  print -u2 "Usage: $0 /path/to/secrets.h"
  exit 1
fi

readonly source_file="$1"
readonly script_dir="${0:A:h}"
readonly destination_dir="${script_dir:h}/data/aws-iot"

if [[ ! -s "${source_file}" ]]; then
  print -u2 "Source file is missing or empty."
  exit 1
fi

mkdir -p "${destination_dir}"

perl -0777 - "${source_file}" "${destination_dir}" <<'PERL'
use strict;
use warnings;

my ($source_path, $destination_dir) = @ARGV;
open my $source, '<', $source_path or die "Cannot open source file\n";
local $/;
my $text = <$source>;

sub quoted_value {
  my ($name) = @_;
  return $1 if $text =~ /(?:\#define\s+\Q$name\E|\b\Q$name\E\b\s*(?:\[\])?\s*=)\s*"([^"]+)"/s;
  die "Required setting is unavailable\n";
}

sub raw_value {
  my ($name) = @_;
  return $2 if $text =~ /\b\Q$name\E\b.{0,200}?R"([A-Za-z0-9_]*)\((.*?)\)\1"/s;
  die "Required setting is unavailable\n";
}

sub write_file {
  my ($name, $value) = @_;
  open my $destination, '>', "$destination_dir/$name" or die "Cannot write destination file\n";
  print {$destination} $value;
  print {$destination} "\n" unless $value =~ /\n\z/;
}

write_file('endpoint.txt', quoted_value('AWS_IOT_ENDPOINT'));
write_file('amazon-root-ca.pem', raw_value('AWS_ROOT_CA'));
write_file('device-certificate.pem', raw_value('DEVICE_CERTIFICATE'));
write_file('private-key.pem', raw_value('DEVICE_PRIVATE_KEY'));
PERL

for required_file in endpoint.txt amazon-root-ca.pem device-certificate.pem private-key.pem; do
  [[ -s "${destination_dir}/${required_file}" ]] || exit 1
done

print "AWS IoT files migrated without displaying their contents."
