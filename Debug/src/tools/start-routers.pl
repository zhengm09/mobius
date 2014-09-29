#!/usr/bin/perl -w

my $num_routers = shift;
my $num_dir_servers = shift;

my $BASE_PATH = "/home/user/mobius/Debug/src";
my $TOR_BINARY = "$BASE_PATH/mobius";
my $data_dir = "$BASE_PATH/routers";

my $tor_path = $TOR_BINARY;
my $window_opts = "";

if(!defined($num_routers) || !defined($num_dir_servers)) {
    print STDERR "Arguments: <num routers> <num dir servers>\n";
    exit;
}
print "Starting inter router \n";
my $cmd = "LD_PRELOAD=/usr/local/lib/libipaddr.so SRCIP=10.0.0.1 $tor_path -i 10.0.0.1 5350 -v > $data_dir/inter/log &";
    print "$cmd\n";
    system("$cmd");

for(my $i =0; $i < $num_dir_servers; $i++) {
print "Starting intra router".$i."\n";
my $intraport = 10000+$i;
my $cmd = "LD_PRELOAD=/usr/local/lib/libipaddr.so SRCIP=10.0.$i.1 $tor_path -n 10.0.$i.1 $intraport 10.0.0.1 5350 -v > $data_dir/intra/$i-log &";
    print "$cmd\n";
    system("$cmd");
}

for(my $i = 1; $i <= $num_routers; $i++) {
    print "Starting router".$i."\n";
   
    sleep(1);
    my $ip = "";
    
        my $find_ip = "ifconfig eth0:$i | grep inet | awk ' {print \$2}' | sed -e 's/addr://'";
        $ip = `$find_ip`;
        chomp $ip;
        
    my $orport = 5000+$i;
    system("mkdir -p $data_dir/$i");
    my $cmd = "LD_PRELOAD=/usr/local/lib/libipaddr.so SRCIP=$ip $tor_path -r $ip $orport 10.0.0.1 5350 -v > $data_dir/$i/log &";
    print "$cmd\n";
    system("$cmd");    
}
