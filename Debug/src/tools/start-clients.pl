#!/usr/bin/perl -w

my $num_clients = shift;
my $num_bulk_clients = shift;
my $client_pause = 2;
my $BASE_PATH = "/home/user/mobius/Debug/src";

if(!defined($num_clients) || !defined($num_bulk_clients)) {
    print STDERR "Arguments: <num clients> <num_bulk_clients>\n";
    exit;
}

for(my $i = 1; $i <= $num_clients; $i++) {
    print "Starting client $i\n";
    my $vap = $i + 40;
    my $find_ip = "ifconfig eth0:$vap | grep inet | awk '{print \$2}' | sed -e 's/addr://'";
    my $ip = `$find_ip`;
    chomp $ip;
    my $host = $ip;
    my $socks_port = 2000 + $i;
    my $mobius_port = 3000 +$i;
    my $bulk = 0;
    if($i <= $num_bulk_clients) {
        $bulk = 1;
    }
    # start tor client
    my $cmd = "$BASE_PATH/tools/run-client.pl $host $socks_port $mobius_port $bulk &";
    print "$cmd\n";
    system("$cmd");
    sleep ($client_pause);
}



