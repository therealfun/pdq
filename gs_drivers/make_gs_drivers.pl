#! /usr/bin/perl

while (<>) {
   chop;
   while ('\\' eq substr ($_, -1, 1) ) {
      chop;
      $_ .= <>;
      chop;
   }
   process_line ($_);
}
print_driver() if (defined $DRIVER_NAME);

sub process_line {
   $_ = shift;
   my (@stuff) = split /\s*:\s*/;
   unless ($stuff[0] eq "choice") {
      if ((defined $OPTIONS) && ($unclosed_options == 1)) {
         $OPTIONS .= "\t}\n\n";
	 $unclosed_options = 0;
      }
   }
   if ($stuff[0] eq "driver") {
      if (defined $DRIVER_NAME) {
         print_driver();
	 undef $OPTIONS;
	 undef $GS_OPTIONS;
	 undef $ARGS;
	 undef $GS_ARGS;
	 undef $DRIVER_HELP;
	 undef $option_count;
	 undef $mode_count;
      }
      $GS_DRIVER_NAME = $stuff[1];
      $DRIVER_FILE = $stuff[2];
      $DRIVER_NAME = $stuff[2];
      $DRIVER_NAME =~ s/-[^-]+$//;  # strip version number
   } elsif ($stuff[0] eq "argument") {
      $stuff[2] =~ s/^-a//;
      $stuff[5] =~ s/\t/\n\t\t\t\t/g;
      $ARGS .= <<EOF;
	argument {
		var = "$stuff[2]"
		desc = "$stuff[4]"
		help = "$stuff[5]"
		def_value = "$stuff[3]"
	}			
EOF
     $GS_ARGS .= "\t\t\t\t$stuff[1]=\$$stuff[2] \\\n";
   } elsif ($stuff[0] eq "option") {
      $unclosed_options = 1;
      $option_count++;
      $OPTIONS .= <<EOF;
	option {
		desc = "$stuff[1]"
		var = OPTION_$option_count

EOF
      unless ($stuff[2] eq '') {
         $OPTIONS .= "\t\tdefault_choice \"$stuff[2]\"\n\n";
      }
      $GS_OPTIONS .= "\t\t\t\t\$OPTION_". $option_count ." \\\n";
   } elsif ($stuff[0] eq "choice") {
      $stuff[2] =~ s/^-o//;
      $stuff[4] =~ s/\t/\n\t\t\t\t/g;
      if ($stuff[2] eq "NAMELESS") {
         $mode_count++;
	 $stuff[2] = "mode".$mode_count;
      }
      $OPTIONS .= <<EOF;
		choice "$stuff[2]" {
			value = "$stuff[1]"
			desc = "$stuff[3]"
			help = "$stuff[4]"
		}
EOF
   } elsif ($stuff[0] eq "resolutions") {
      process_line ( "option : resolution" );
      for ($i = 1; $i < @stuff; $i++ ) {
         $res = $stuff[$i];
	 next if ($res eq '');
         process_line ( "choice : -r$res : -o$res : $res : Print at $res dpi" );
      }
   } elsif ($stuff[0] eq "help") {
      $DRIVER_HELP .= $stuff[1] . "\n\t\t";
   } else {
   }

}

##############################################################################################
##
##   pdq driver follows:
##
##############################################################################################

sub print_driver {

chop ($DRIVER_HELP);
chop ($DRIVER_HELP);
chop ($DRIVER_HELP);

open (OUT, ">$DRIVER_FILE") || die;

print OUT<<EOF;
driver "$DRIVER_NAME" {
   
	help "$DRIVER_HELP"

	requires "gs"

	verify_exec {
		(gs -h |grep -q $GS_DRIVER_NAME) || 
		(echo "Ghostscript driver $GS_DRIVER_NAME is required"; exit 1)
	}

$OPTIONS

	option {
		var = FIX_STAIRCASE;
		desc = "staircase text correction"
		choice scyes {
			value = "YES"
			desc = "fix staircased text problems"
			help = "Fixes printouts\\n\\t\\tthat look\\n\\t\\t\\t\\tlike this!"
		}
		choice scno {
			value = "NO"
			desc = "leave text alone"
		}
	}

$ARGS

	language_driver postscript {}    # Nothing to do...

	language_driver text {
		convert_exec = {
		     if [ "\$FIX_STAIRCASE" = "YES" ]; then
			sed 's/\$/\r/' \$INPUT > \$OUTPUT;
		     else
			ln -s \$INPUT \$OUTPUT;
		     fi
		     touch \$OUTPUT.ok;  # Tell filter_exec not to invoke gs
		}
	}

	filter_exec {
		if ! test -e \$INPUT.ok; then
         		gs \\
             			-q -dBATCH -dSAFER -dNOPAUSE \\
				-sDEVICE=$GS_DRIVER_NAME \\
	     			-sOutputFile=\$OUTPUT \\
$GS_ARGS$GS_OPTIONS				\$INPUT
	     		if ! test -e \$OUTPUT; then 
	        		echo "ABORTED: Ghostscript had no output." >> \$STATUS
				exit 1
             		fi
             	else
			ln -s \$INPUT \$OUTPUT;
             	fi
	}

}
EOF

close OUT;

}
