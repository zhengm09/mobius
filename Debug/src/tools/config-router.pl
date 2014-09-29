#!/usr/bin/perl -w

my $if = shift;
my $dir_fingerprint_file = shift;
my $exit_policy = shift;
my $directory = shift;
my $avg_bw = shift;
my $burst_bw = shift;

my $MAX_BW = 10 * 1024; # in KB/s
my $BASE_PATH = "~/mobius/Debug/src";

if(!defined($if) || !defined($dir_fingerprint_file) || !defined($exit_policy) ||
   !defined($directory) || !defined($avg_bw) || !defined($burst_bw)) {
    print STDERR "Arguments: <interface #> <dir lines file> <exit policy (1 or 0)> <directory (1 or 0)> <bw avg> <bw burst>\n";
    exit;
}

#my $find_ip = "ifconfig eth0:$if | grep inet | awk '{print \$2}' | sed -e 's/addr://'";
#my $ip = `$find_ip`;
my $ip = "10.0.0.$if";
chomp $ip;

my $orport = 5000 + $if;
my $socksport = 7000 + $if;

my $me = `whoami`;
chomp $me;

my $torrc = "";
$torrc .= "Address $ip\n";
$torrc .= "ORPort $orport\n";
$torrc .= "ORListenAddress $ip:$orport\n";
$torrc .= "SocksPort $socksport\n";

if($avg_bw > $MAX_BW) {
    $avg_bw = $MAX_BW;
}

if($burst_bw > $MAX_BW) {
    $burst_bw = $MAX_BW;
}

if($directory != 0) {
    $torrc .= "DirPort ".(10000 + $if)."\n";
    $torrc .= "DirListenAddress 10.0.0.".$if.":".(10000 + $if)."\n";  
}

my $data_dir = "$BASE_PATH/routers/$if";
system("mkdir -p $data_dir");
system("echo \'$torrc\' > $data_dir/torrc");

