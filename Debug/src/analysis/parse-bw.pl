#! /usr/bin/perl -w 

use Date::Manip qw(ParseDate UnixDate); 
use Getopt::Std;
use Switch;

our( $opt_m, $opt_d , $opt_t);

sub usage {
	$reason = shift;
	if (defined($reason)){
		print STDERR "Error: $reason\n";
	}
	print STDERR "./parse-bw.pl [-m <keep last x minutes>] -d <data directory> [-t <type of client (web|bulk)]\n";
	exit(2);
}

getopts("m:d:t:");

usage("Please provide data directory") if (!defined($opt_d));

usage("Data Directory not found") if (! (-d "$opt_d"));
usage("Please provide a valid type") if (defined($opt_t) and !(($opt_t eq "bulk") || ($opt_t eq "web")));



my $end_time = `grep stop $opt_d/summary | awk '{print \$7}'`;

my $last_minutes = (defined($opt_m)) ? $opt_m*60 : 30*60;

my @dirs;

if (defined($opt_t)){
  	# Parse a specific type
	@dirs = `find $opt_d -name type-$opt_t -exec dirname {} \\;`;
} else {
	@dirs = `ls $opt_d/clients/* -d`;
}

my $avg_speed = 0;

foreach my $dir (@dirs) {
	chomp $dir;
	my $file = "$dir/wget";
    my $total_bytes = 0;
	my $counter = 0;
	my $processing = 0;
	my $total_wait =0;
		my ($speed, $speed_type);
	   	my $startstring; 
		my @wget_reported_time;
    open( F, "$file" ) or die "Cannot open $file\n";
#	print STDERR "Processing $file\n";
    while( <F> ) {
		chomp;


        if ($_ =~ /--([-0-9]+ [:0-9]+)--/)
		{
		#	print "Found: $_\n";
			$processing = 1;
			$startstring = $1;
			@wget_reported_time = ();
		}

		if ($_ =~ /.*100%.*=([0-9\.]+.)(?:([\.0-9]+.)?)/)
		{
			my $matched = $+;
			my $counter1 = 0;
			$counter1++ while ($matched =~ m/[0-9\.]+./g);
			# We found a completion timestamp while processing
			 $_ =~ /.*100%.*=([0-9\.]+.)(?:([\.0-9]+.)?)/;
			for (my $i = 1; $i < $counter1+1; $i++){
				push(@wget_reported_time, $$i);
			}
		}

		if ($_ =~ /^(.+) \(.+ saved \[(.+)\//){
        #   print "Found close: $_\n";
		} else { next;}		

		die("Didn't find wget reported line before line: $_\n") if (@wget_reported_time eq "");

        # Wget reports times as "1m45s", etc
		my $wget_converted_time;
		foreach my $entity (@wget_reported_time){
        	$entity =~ /([0-9\.]+)(.+)/;
			$wget_reported_unit = $2;
			if ($wget_reported_unit eq "s"){
				$wget_converted_time += $1;
			} elsif ($wget_reported_unit eq "m"){
				$wget_converted_time += $1*60;
			} else {
				die("Did not recognize time unit '$wget_reported_unit'\n");
			}
        }

		$processing = 0;
		my $endstring = $1;
		my $bytes = $2;

		my $download_endtime = UnixDate($endstring, "%s");
		my $download_starttime = UnixDate($startstring, "%s");
		
                                                                
		my $ttl = ($download_endtime - $download_starttime);

		if ($ttl > 0){
			$speed = $bytes/ $ttl;
		} else {
			next;
		}

		my $secs = UnixDate($endstring, "%s");

		#$ttl represents the total time spent in a wget attempt,including connection overhead.
		# $wget_converted_time is the amount of time reported by wget, which doesn't include overhead
		# Thus the difference between the two is the overhead.
		$total_wait += $ttl - $wget_converted_time;

		$total_bytes += $bytes if ($end_time - $secs <= $last_minutes);
		$avg_speed += $speed if ($end_time - $secs <= $last_minutes);

		$counter +=1 if ($end_time - $secs <= $last_minutes);
    }

	$avg_speed = ($counter > 0 ) ? $avg_speed/ $counter : 0;

	close( F );
	print "$avg_speed $total_bytes $total_wait\n";
}
