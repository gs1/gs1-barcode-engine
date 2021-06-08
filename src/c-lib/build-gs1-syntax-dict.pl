#!/usr/bin/perl -Tw

#
#  cat gs1-format-spec.txt | ./build-gs1-syntax-dict.pl
#

use strict;

my $ai_rx = qr/
    (
        (0\d)
    |
        ([1-9]\d{1,3})
    )
/x;

my $ai_rng_rx = qr/${ai_rx}(-${ai_rx})?/;

my $flags_rx = qr/[\*]+/;

my $type_rx = qr/
    [XNC]
    (
        ([1-9]\d?)
        |
        ([0-9]\d?\.\.[1-9]\d?)
    )
/x;

my $comp_rx = qr/
    ${type_rx}
    (,\w+)*
/x;

my $spec_rx = qr/
    ${comp_rx}
    (\s+${comp_rx})*
/x;

my $keyval_rx = qr/
    (
        (\w+)
        |
        (\w+=\S+)
    )
/x;

my $title_rx = qr/\S.*\S/;

# 999  *  N13,csum,key X0..17  dlpkey=22,10  # EXAMPLE TITLE
my $entry_rx = qr/
    ^
    (?<ais>${ai_rng_rx})
    (
        \s+
        (?<flags>${flags_rx})
    )?
    \s+
    (?<spec>${spec_rx})
    (
        \s+
        (?<keyvals>
            (${keyval_rx}\s+)*
            ${keyval_rx}
        )
    )?
    (
        \s+
        \#
        \s
        (?<title>${title_rx})
    )?
    \s*
    $
/x;

while (<>) {

    chomp;

    $_ =~ /^#/ and next;
    $_ =~ /^\s*$/ and next;

    $_ =~ $entry_rx or die;

    my $ais = $+{ais};
    my $flags = $+{flags} || '';
    my $spec = $+{spec};
    my $keyvals = $+{keyvals} || '';  # ignored
    my $title = $+{title} || '';

    my @elms = split(/\s+/, $spec, 5);
    $#elms = 4;

    my $specstr = '';
    foreach (@elms) {

        if (!defined($_)) {
            $specstr .= ' __,';
            next;
        }

        (my $cset, my $checks) = split(',', $_, 2);

        ($cset, my $len) = $cset =~ /^(.)(.*)$/;
        $len = "1$len" if $len =~ /^\.\./;
        $len = "$len..$len" if $len !~ /\./;
        (my $min, my $max) = $len =~ /^(\d+)\.\.(\d+)$/;

        $checks=$checks || '';
        my @checks=split(',', $checks, 2);
        $#checks = 0;
        $checks='';
        foreach (@checks) {
            $_ = '_' unless (defined $_ && $_ eq 'csum');   # csum only for the moment
            $checks .= "$_,";
        }
        $checks =~ s/^\s+|\s+$//g;

        $specstr .= " $cset,$min,$max,$checks";

    }

    $ais = "$ais-$ais" if $ais !~ /-/;
    (my $aimin, my $aimax) = $ais =~ /^(\d+)-(\d+)$/;

    my $fnc1 = $flags =~ /\*/ ? 'NO_FNC1' : 'FNC1   ';

    $specstr = sprintf("%-46s", $specstr);

    $title =~ s/²/^2/;
    $title =~ s/³/^3/;
    $title = sprintf("%-27s", "\"$title\"");

    for ($aimin..$aimax) {
        $_ = sprintf('%-6s', "\"$_\"");
        print "AI( $_, $fnc1,$specstr$title ),\n";
    }

}
