#!/usr/bin/perl -w

my $num_routers = shift;
my $num_directories = shift;
my $dir_line = shift;
my $tor_model_file = shift;

my $i = 0;
my $scale = 0;
my $real_count = 0;
my %percentiles = ();
my $MAX_BW = 10 * 1024; # in KB/s
my $MIN_BW = 20;        # in KB/s
my $BASE_PATH = "~/mobius/Debug/src";
my $me = `whoami`;
chomp $me;

if(!defined($num_routers) ||
   !defined($num_directories) ||
   !defined($dir_line) ||
   !defined($tor_model_file)) {
    print STDERR "Arguments: <num routers> <num directories> <torrc \"DirServ\" lines> <real tor model file>\n";
    exit;
}

open(FD, "$tor_model_file") or die "Cannot open file $tor_model_file\n";
while(<FD>) {
    chomp;
    my ($fp, $avg_bw, $burst_bw, $flags) = split (/;/);
    $avg_bw /= 1024;
    $burst_bw /= 1024;

    if($avg_bw > $MAX_BW) {
        $avg_bw = $MAX_BW;
    }

    if($burst_bw > $MAX_BW) {
        $burst_bw = $MAX_BW;
    }

    if($avg_bw < $MIN_BW) {
        $avg_bw = $MIN_BW;
    }
    if($burst_bw < $MIN_BW) {
        $burst_bw = $MIN_BW;
    }
    $percentiles {$i} = "$fp;".int($avg_bw).";".int($burst_bw).";$flags";
    $i++;
}
close(FD);

$real_count = $i;
$scale = $i / $num_routers;

open(FD, ">/home/$me/~/mobius/Debug/src/exp.model") 
    or die "Cannot open exp.model: $!";
print FD "scale = $scale\n";
for(my $i = 1; $i <= $num_routers; $i++) {
    my ($fp, $avg_bw, $burst_bw, $flags) = split(/;/, $percentiles {int(($i-1) * $scale)});
    my $directory = 0;
    my $exit_policy = 0;
    my $percentile = int(100 - (100 * ($i-1) * $scale) / $real_count);

    if($i <= $num_directories) {
        $directory = 1;
    }

    $exit_policy = 1;

    print FD "Router $i Percentile $percentile $fp $avg_bw $burst_bw $exit_policy $flags\n";
    system("$BASE_PATH/tools/config-router.pl $i $dir_line $exit_policy $directory $avg_bw $burst_bw\n");
}
close(FD);
