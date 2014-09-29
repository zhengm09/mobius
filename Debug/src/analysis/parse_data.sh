#!/bin/bash

function script_valid() {
   if [ ! -x $1 ] 
   then
	   	echo "Can't run script: $1";
		return -1
	fi
	return 0
}

function R-cli-bw-dat {

	if script_valid "$script_dir/parse-bw.pl"
		then
		for d in $DIRLIST;
		do
			echo -n "."
			$script_dir/parse-bw.pl -m 45 -d "$d"  >"${outdir}R-cli-bw_${d}.Rdat"
		done
	fi
}

function bulk-R-cli-bw-dat {
	if script_valid "$script_dir/parse-bw.pl"
		then
		for d in $DIRLIST;
		do
			echo -n "."
			$script_dir/parse-bw.pl -m 45 -d "$d"  -t bulk >"${outdir}bulk-R-cli-bw_${d}.Rdat"
		done
	fi
}

function web-R-cli-bw-dat {
	if script_valid "$script_dir/parse-bw.pl"
		then
		for d in $DIRLIST;
		do
			echo -n "."
			$script_dir/parse-bw.pl -m 45 -d "$d" -t web >"${outdir}web-R-cli-bw_${d}.Rdat"
		done
	fi
}

function cli-bw-cdf {
	echo "Generating client measured bandwidth cdf data files";	
	if script_valid "$script_dir/cdf.pl" 
		then
		for d in $DIRLIST; 
		do 
			echo -n "."  
			grep "KB/s" $d/clients/*/wget | cut -d " " -f 3 | cut -d "(" -f 2 | sort -n | $script_dir/cdf.pl >  "${outdir}cli-measured-bw_${d}.cdf"
		done
	fi
}

function bulk-cli-bw-cdf {
	echo "Generating measured bandwidth CDFs for bulk clients";
	if script_valid "$script_dir/cdf.pl" 
		then
		for d in $DIRLIST;
		do
			echo -n "*"
			bulk=`find $d -name type-bulk | sed 's|\(.*/\).*|\1|'`
			tmp=/tmp/bulk-bw-cdf.tmp${RANDOM}
			cat /dev/null > $tmp
			for cli in $bulk;
			do
				echo -n "."
				grep "KB/s" $cli/wget | cut -d " " -f 3 | cut -d "(" -f 2 >> $tmp
			done
			cat $tmp | sort -n | $script_dir/cdf.pl >  "${outdir}bulk-cli-measured-bw_${d}.cdf"
			rm $tmp
		done
		echo ""
	fi
}

function web-cli-bw-cdf {
	echo "Generating measured bandwidth CDFs for web clients";
	if script_valid "$script_dir/cdf.pl" 
		then
		for d in $DIRLIST;
		do
			echo -n "*"
			tmp=/tmp/web-bw-cdf.tmp${RANDOM}
			cat /dev/null > $tmp
			web=`find $d -name type-web | sed 's|\(.*/\).*|\1|'`
			for cli in $web;
			do
				echo -n "."
				grep "KB/s" $cli/wget | cut -d " " -f 3 | cut -d "(" -f 2 >> $tmp
			done
  			cat $tmp | sort -n | $script_dir/cdf.pl >  "${outdir}web-cli-measured-bw_${d}.cdf"
	   		rm $tmp 
		done
		echo ""
	fi
}

asksure() {
echo -n "Continue (Y/N)? "
while read -r -n 1 -s answer; do
  if [[ $answer = [YyNn] ]]; then
    [[ $answer = [Yy] ]] && retval=0
    [[ $answer = [Nn] ]] && retval=1
    break
  fi
done

echo # just a final linefeed, optics...

return $retval
}

handle_opt() {
	case $1 in
		'--log')
			LOG="$2"
			shifts=1
			;;
		'--output-dir')
			outdir=$2
			[[ $outdir != */ ]] && outdir="$outdir"/
			shifts=1
			;;
		'--email')
			email=$2
			shifts=1
			;;
		'--help')
			ext_usage
			;;
	esac
}


usage() {
	echo "Usage: parse_data.sh <script_dir> [options] command "
	echo
	echo "  <script_dir> must be a directory containing any required plotting scripts"
	echo " 	Run with --help to see required scripts"
	echo
	echo "	Commands (provide a space separated list, or 'all'):"
	echo " 		R-cli-bw-dat"
	echo " 		bulk-R-cli-bw-dat"
	echo " 		web-R-cli-bw-dat"
	echo "		cli-bw-cdf"
	echo "		bulk-cli-bw-cdf"
	echo "		web-cli-bw-cdf"
	echo 
	echo "	Options:"
	echo "		--email <recipient>	Send email to <recipient> when finished"
	echo "		--output-dir <dir>	Directory to output data files to"
	echo "		--log <file>	File to write verbose output to"
	echo "		--help 	Print extended information about commands"
	
	exit 2
}

ext_usage() {
 	echo "Extended Command Information:"
	echo 
	echo "	[web|bulk|]-R-cli-bw-dat"
	echo "		- Parse client bandwidth data, taking into account connection time"
	echo "		- Depends: parse-bw.pl"
	echo
	echo "	[web|bulk|]-cli-bw-cdf"
	echo "		- Parse client bandwidth data into a CDF based on wget reported speed."
	echo "		- Depends: cdf.pl"
	echo ""

	exit 2
}

##########################
# MAIN STARTS HEREE
#########################

if [ $# -lt 1 ]; then 
	usage
fi  

if [[ $1 == '--help' ]]; then
	ext_usage
fi

script_dir=$1;
if [ ! -d "$script_dir" ]; then
	echo "Analysis script dir couldn't be located. Exiting" 
	exit -1
fi
shift #shift script_dir


echo "This tool will perform the requested data runs on ALL directories in the directory where it is run from."
echo "Make sure that those are the directories you want before continuing."
if ! asksure; then 
	exit 3
fi

DIRLIST=`ls -p | egrep '.*/' | sed 's/\(.*\)\//\1/'`
OUTPUT_FILE="> /dev/null 2>&1"
saved_args=$@
LOG="/dev/null"

while [ $# -ne 0 ]; do
    if [ $outdir ];
		then
		#This is clever - make output dir after we build DIRLIST
		[[ ! -d $outdir ]] && mkdir -p $outdir
    fi
	
	case $1 in 
	--*) 
		if [ ! $END_OPT ];
		then
			#Let's catch us some options
			shifts=0
			echo "Found option: $1"
			handle_opt "$@"
			while [ "$shifts" -ne 0 ]; do shift; shifts=$[$shifts-1]; done
		fi
		;;
			
	'all')
		END_OPT=1
		cli-bw-cdf
		bulk-cli-bw-cdf
		web-cli-bw-cdf
		R-cli-bw-dat
		bulk-R-cli-bw-dat
		web-R-cli-bw-dat
		;;
		
	'R-cli-bw-dat')
		END_OPT=1
		R-cli-bw-dat
		;;
	'bulk-R-cli-bw-dat')
		END_OPT=1
		bulk-R-cli-bw-dat
		;;
	'web-R-cli-bw-dat')
		END_OPT=1
		web-R-cli-bw-dat
		;;
	'cli-bw-cdf')
		END_OPT=1
		cli-bw-cdf
		;;
    'web-cli-bw-cdf')
		END_OPT=1
		web-cli-bw-cdf
		;;
    'bulk-cli-bw-cdf')
		END_OPT=1
		bulk-cli-bw-cdf
		;;
		
	*)
		END_OPT=1
		usage
		;;

	esac

	shift

done

if [ ! $END_OPT ]; then
	usage
fi

[[ $email ]] && mail -s "$(hostname) - Data Run Complete" $email <<EOM
Run Completed on $(hostname) at $(date)
EOM

