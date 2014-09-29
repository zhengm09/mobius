#!/usr/bin/perl -w

my $file = shift;
my $socks_host = shift;
my $socks_port = shift;
my $timeout = 60 * 10; # in s
my $BASE_PATH = "~/mobius/Debug/src";

if(!defined($file) || !defined($socks_host) || !defined($socks_port)) {
    print STDERR "Arguments: <remote file> <socks_host> <socks_port>\n";
    exit;
}

srand(time);

while(1) {
    my $rs = int(rand(80));
    my $cmd = "timeout $timeout $BASE_PATH/torperf/trivsocks-client 10.0.4.$rs $socks_host:$socks_port /$file.file >> $BASE_PATH/torperf/$file.out 2> /dev/null";
    system("$cmd");
}
