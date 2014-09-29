#!/usr/bin/perl -w

srand(time);

my $socks_host = shift;
my $socks_port = shift;
my $bulk = shift;

my $BASE_PATH = "/home/user/mobius/Debug/src/tools";
my $MAX_PAUSE_HTTP = 20;
my $MAX_PAUSE_BULK = 1;
my $MAX_HTTP_STREAMS = 0;
my $MAX_BULK_STREAMS = 0;
my $tmp_file = "";

my @http_files = ("100KB.file", "200KB.file", "300KB.file", "400KB.file", "500KB.file");
my @bulk_files = ("1MB.file", "1MB.file","1MB.file", "1MB.file","1MB.file");

sub pick {
    my @files = @_;
    my $filename = "";
    my $r = rand();

    if($r < 0.2) {
        $filename = $files[4];
    }
    elsif($r < 0.4) {
        $filename = $files[3];
    }
    elsif($r < 0.6) {
        $filename = $files[2];
    }
    elsif($r < 0.8) {
        $filename = $files[1];
    }
    else {
        $filename = $files[0];
    }
    return "$filename";
}

if(!defined($socks_host) || !defined($socks_port) || !defined($bulk)) {
    print STDERR "Arguments: <socks host> <socks port> <bulk ?>\n";
    exit;
}

# wait for tor client to start
sleep (int(rand() * $MAX_PAUSE_HTTP));

while(1) {
    my $filename = "";
    my $num_streams = 0;

    if($bulk == 0) {
        $num_streams = 1 + int(rand($MAX_HTTP_STREAMS));
    } else {
        $num_streams = 1 + int(rand($MAX_BULK_STREAMS));
    }
    $tmp_file = "/tmp/mobius-http-client-$socks_host.$socks_port";
    open(FD, "> $tmp_file")
    or die "Cannot open file $tmp_file\n";
    for(my $i = 0; $i < $num_streams; $i++) {
        my $size = 0;
        if($bulk == 0) {
            $filename = pick(@http_files);
#$filename = "300KB.file";
        } else {
            $filename = pick(@bulk_files);
#$filename = "5MB.file"
        }
        my $rand_host = 50 + int(rand(50));
        print FD "http://10.0.0.$rand_host/$filename\n";
    }
    close(FD);

    my $cmd = "$BASE_PATH/torify.pl $socks_host $socks_port \'$BASE_PATH/pget.py < $tmp_file 2>> /tmp/mobius-client-$socks_host:$socks_port/wget\'";
    system("$cmd");
    print "$cmd\n";
    my $pause = 0;
    if($bulk == 0) {
        $pause = 1 + int(rand() * $MAX_PAUSE_HTTP);
    } else {
        $pause = 1 + int(rand() * $MAX_PAUSE_BULK);
    }
    sleep $pause;
}
