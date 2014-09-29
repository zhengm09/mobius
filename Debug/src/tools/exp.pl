#!/usr/bin/perl -w

my $BASE_PATH = "/home/user/mobius/Debug/src";


my $NUM_DIRS = 1;

sub stop {
    my $cmd = "";

    # stop clients
    $cmd = "$BASE_PATH/tools/stop-clients.pl";
    print "Stopping clients\n";
    system($cmd);

    # stop routers
    $cmd = "$BASE_PATH/tools/stop-routers.pl";
    print "Stopping routers\n";
    system($cmd);

    # add the experiment's stop time to the summary 
    open(FD, ">>summary") or die "Cannot open summary\n";
    print FD "stop: ".localtime(time)." ".time."\n";
    close(FD);
}

sub start {
    my $cmd = "";
    my $total_routers = shift;
    my $num_dir = shift;
    my $total_clients = shift;
    my $total_bulk_clients = shift;
    my $WAIT = 60;

    # write experiment summary file
    open(FD, ">summary") or die "Cannot open summary\n";
    print FD "$total_routers $num_dir $total_clients $total_bulk_clients\n";
    print FD "start: ".localtime(time)." ".time."\n";
    close(FD);

    

    # start routers
    $cmd = "$BASE_PATH/tools/start-routers.pl $total_routers $num_dir &";
    print "Starting routers: $cmd\n";
    system($cmd);
    sleep $WAIT;

    # start clients
    $cmd = "$BASE_PATH/tools/start-clients.pl $total_clients $total_bulk_clients &";
    print "Starting clients\n";
    print "$cmd\n";
    system($cmd); 

   # start http
    $cmd = "$BASE_PATH/tools/start-http.pl $total_clients $total_bulk_clients &";
    print "Starting http\n";
    print "$cmd\n";
    system($cmd); 
}

sub data {
    my $cmd = "";
    my $time = time;
    chomp $time;
    my $router_dir = "$BASE_PATH/data/$time/routers";
    my $client_dir = "$BASE_PATH/data/$time/clients";  
    system("mkdir -p $router_dir");
    system("mkdir -p $client_dir");


    # get router data
    print "Copying router logs\n";
    $cmd = "cp -r $BASE_PATH/routers/* $router_dir";
    system($cmd);

    # get client data
    print "Copying client logs\n";
    $cmd = "cp -r /tmp/mobius-client-* $client_dir";
    system($cmd);    
    $cmd = "rm -R /tmp/mobius-client-* ";
    system($cmd);    

    # get experiment summary file
    print "Copying experiment summary file\n";
    $cmd = "cp summary $BASE_PATH/data/$time";
    system($cmd);   
}

my $cmd = $ARGV[0];
my $usage = "./exp.pl start <number of routers> <total number of clients> <number of bulk clients> | stop | data";

if(lc($cmd) eq "stop") {
    stop();
}
elsif(lc($cmd) eq "start") {
    if($#ARGV != 3 && $#ARGV != 6) {
        print STDERR "$usage\n";
    }
    else {
        my $num_routers = $ARGV[1];
        my $num_dirs = $NUM_DIRS;
        my $num_clients = $ARGV[2];
        my $num_bulk_clients = $ARGV[3];
        start($num_routers, $num_dirs, $num_clients, $num_bulk_clients);
    }
} elsif(lc($cmd) eq "data") {
    my $num_routers = $ARGV[1];
    data();
} else {
    print STDERR "$usage\n";
}
