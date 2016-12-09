#!/usr/bin/perl

use v5;
use strict;
use warnings;

my $matchstring;
my $matchr;
my $acktime;
my $ackcount;
my $ackplus;
my $ackminus;

open(FH,"<","node1long.tr") or die "error happened: can't open file: $!";
open(QH,">","SEQ+.dat") or die "error happened: can't open file: $!";


while(<FH>){
	$matchstring = $_;
	if($matchstring =~ /^\+/){
		$matchr = $';
		$matchr =~ /\d+\.\d+|\d+/;
		$acktime = $&;
		print QH $acktime,"\t\t";
		if($matchr =~ /ns3::TcpHeader\s.*/){
			$ackcount = $&;
			$ackcount =~ s/.*Seq=(\d+)\sAck=.*/$1/;
			$ackcount = $ackcount / 512;
			$ackcount = $ackcount % 60;
			print QH $ackcount,"\n";

		}
	} 
}

close FH;
close QH;