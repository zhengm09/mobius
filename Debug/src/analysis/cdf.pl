#!/usr/bin/perl

@lines = <STDIN>;
$num = @lines;

$c = 0;
foreach $ln (@lines) {
    chop($ln);
    $c++;
    $per = 100*$c / $num;
    print $per . " " . $ln . "\n";
}
