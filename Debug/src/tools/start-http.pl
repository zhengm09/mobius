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
    my $port = 2000 + $i;

    # start http client
    my $bulk = 0;
    if($i <= $num_bulk_clients) {
        $bulk = 1;
    }
    my $cmd = "$BASE_PATH/tools/http-client.pl $host $port $bulk &";
    print "starting http $i: $cmd\n";
    system("$cmd");
    # start a new client every 2.5 seconds (Micah)
    sleep ($client_pause);
}
