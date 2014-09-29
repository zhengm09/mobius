#!/usr/bin/perl -w

my $socks_host = shift;
my $socks_port = shift;
my $mobius_port = shift;
my $bulk = shift;

my $BASE_PATH = "~/mobius/Debug/src";
my $Mobius_BINARY = "$BASE_PATH/mobius";

if(!defined($socks_host) || !defined($socks_port)||!defined($mobius_port)||!defined($bulk)) {
    print STDERR "Arguments: <socks host> <socks port> <mobius port> <bulk>\n";
    exit;
}

my $Mobius_path = $Mobius_BINARY;
my $relay=3;
if ($bulk==1){
    $relay=1;    
}



#my $cmd = "LD_PRELOAD=/usr/local/lib/libipaddr.so SRCIP=$socks_host $Mobius_path -c $socks_host $socks_port $mobius_port 10.0.0.1 5350 $relay -v";
#$cmd .= " > /tmp/mobius-client-$socks_host:$socks_port/log";
#print "$cmd\n";

system("mkdir -p /tmp/mobius-client-$socks_host:$socks_port");
system("cat /dev/null > /tmp/mobius-client-$socks_host:$socks_port/log");
system("rm -f /tmp/mobius-client-$socks_host:$socks_port/wget");
system("rm -rf /tmp/mobius-client-$socks_host:$socks_port/cached-*");
system("$cmd");
