#!/usr/bin/perl
use strict;
use threads;
use threads::shared;
use Thread::Semaphore;
use DBI;
use Fraction;
use DBD::SQLite;
use POSIX qw(ceil floor);

#final trace file. It contains at the end of tests, all eperf_tool commands with results.
my $trace_file;

#binary semaphore to protect write access to @tab_cmd
my $semaphore:shared;
# binary semaphore to protect write access into the sqlite file database (if sqlite is used).
my $semaphore_sqlite:shared;
#contains list of eperf_tool commands to exectute
my @tab_cmd:shared;
#used by differents threads for collecting into database.
my @connection_string:shared;

#not buffering
$| = 1;

#path to the parameters file given in argument
my $file_params=shift;
#check if argument is OK
usage() unless $file_params;

my $start = time;
main();
my $end = time;
my $cpt=$end-$start;

print "\nTime : $cpt s.\n";

sub main
{
  #contains for each tests the eperf_tool command its arguments
  my $tool_with_args;
  #we save all parameters for this script in a hash.
  my %params;
  #init differents params before reading file parameters.
  init_params(\%params);
  # read file parameters and fill the hash. it erases params set in init_params.
  read_params($file_params,\%params);
  
  #check if number of source symbols and repair symbols are correct.
  if (not defined @{$params{"code_rate"}} and scalar @{$params{"nb_source_symbols"}} != scalar @{$params{"nb_repair_symbols"}})
  {
    print "bad number of nb_repair_symbols arg or nb_source_symbols arg. Exiting !\n";
    return;
  }

  #the database handle
  my $dbh;

  # all params used for tests
  my $database_type=  @{$params{"database"}}[0];
  my $erase_database = @{$params{"erase_database"}}[0];
  my $tool=@{$params{"fec_tool"}}[0];
  my $qc2mod2sparse_tool=@{$params{"qc2mod2sparse"}}[0];
  $trace_file = @{$params{"trace_file"}}[0];
  my $left_degree;
  my $fec_ratio;
  my $codec;
  my $symbol_size;
  my $r;
  my $k;
  my $num_k;
  my $ss;
  my $tx_type;
  my $loss;
  my $iter = @{$params{"nb_iterations"}}[0];
  my $counter=0;
  my $exp_factor=0;
  my $nb_thread = get_nb_cpu()+1;
  # you can set manually the number of wanted threads : 
  #$nb_thread=2;
  
  #here, we only work with one thread (using_thread different from true)
  if ( not @{$params{"using_threads"}}[0] eq "true")
  {
    $nb_thread = 1;
  }
  #only one thread can access to @tab_cmd and shift it !
  $semaphore = Thread::Semaphore->new;
  # matrix mode for QC-LDPC codecs : by default, it's binary matrices
  my $matrix_mode="binary";
  if (  @{$params{"matrix_mode"}}[0] eq "qc")
  {
      $matrix_mode="qc";
  }
  
  #used by the differents threads
  my $nb_tmp_files=0;

  my $os = get_os();
  my $hostname = `hostname`;
  my $uname = `uname -a`;
  
  my $nb_tests;
  my $num_test=1;

  #erase the final trace file
  unlink $trace_file;
  

  #set the database type, create tables, and set the connexion
  if ($database_type eq "file")
  {
    print "Database type : File\n";
	  # only one thread can access to sqlite database file.
	  $semaphore_sqlite = Thread::Semaphore->new;
    if ($erase_database eq "true")
    {
      print "Erasing database File\n";
      #erase the database file if exists
      unlink @{$params{"database"}}[1] if (-e @{$params{"database"}}[1]);
      #open a connexion to the database file
      @connection_string = ("dbi:SQLite:dbname=".@{$params{"database"}}[1],"","");
      $dbh = DBI->connect("dbi:SQLite:dbname=".@{$params{"database"}}[1],"","");
      print "Creating tables\n";
      create_database_template($dbh,"sqlite");
      init_database($dbh);
    }
    else
    {
      #open a connexion to the database file
      @connection_string =("dbi:SQLite:dbname=".@{$params{"database"}}[1],"","");
      $dbh = DBI->connect("dbi:SQLite:dbname=".@{$params{"database"}}[1],"","");
      print "Creating tables\n";
      create_database_template($dbh,"sqlite");
      if (get_codec_id($dbh,"LDPC-Staircase") eq "3")
      {
	print "Fill codec table\n";
	init_database($dbh);
      }
    }
  }
  else
  {
    if ($database_type eq "server")
    {
	print "Database type : Server\n";
	@connection_string =("dbi:mysql:database=".@{$params{"database"}}[1].";host=".@{$params{"database"}}[2].";port=".@{$params{"database"}}[3],@{$params{"database"}}[4],@{$params{"database"}}[5]);
  	#open a connexion to the database file
	$dbh = DBI->connect("dbi:mysql:database=".@{$params{"database"}}[1].";host=".@{$params{"database"}}[2].";port=".@{$params{"database"}}[3],@{$params{"database"}}[4],@{$params{"database"}}[5]);
	if ($erase_database eq "true")
	{
	  print "Erase database on server\n";
	  erase_database($dbh);	  
	print "Creating tables\n";
	create_database_template($dbh,"mysql");
	if (not (get_codec_id($dbh,"LDPC-Staircase") eq "3"))
	{
	  print "Fill codec table\n";

	  init_database($dbh);
	}
	  
	}
	else
	{
		print "Creating tables\n";
		create_database_template($dbh,"mysql");
		if (not (get_codec_id($dbh,"LDPC-Staircase") eq "3"))
		{
		  print "Fill codec table\n";

		  init_database($dbh);
		}
	}
    }
    else
    {
	  print "Wrong database type. Exiting !\n";
	  return;
    }
  }

  
  system("echo '--------------------------------' > $trace_file");
  system("echo 'Hostname=$hostname  OS=$os' >> $trace_file");
  system("echo 'uname=$uname' >> $trace_file");
  system("echo '--------------------------------\n' >> $trace_file");
 
  #get list of number of source symbols values with min max step scheme.
  my @list_of_k = get_list_of_values(\@{$params{"nb_source_symbols"}});
  my @list_of_r;

  my @list_of_left_degrees;
  if (defined @{$params{"ldpc_N1"}})
  {
    @list_of_left_degrees = get_list_of_values(\@{$params{"ldpc_N1"}});
  }
  my @list_of_exp_factor;
  if (defined @{$params{"exp_factor"}})
  {
    @list_of_exp_factor = get_list_of_values(\@{$params{"exp_factor"}});
  }
  #code_rate param is defined ( scalar > 1 <=> size > 0 ), we organize both nb_repair_symbols and nb_source_symbols tabs.
  if (defined @{$params{"code_rate"}})
  {
     my $k_range=0;;
     @{$params{"nb_source_symbols"}} = ();
     foreach my $ratio (@{$params{"code_rate"}})
     {
     		$ratio=Math::Fraction::frac($ratio);
		foreach  my $copy_k (@list_of_k)
		{
			@{$params{"nb_source_symbols"}}[$k_range] = int($copy_k);
			@{$params{"nb_repair_symbols"}}[$k_range] =  int($copy_k * 1/$ratio)-int($copy_k);
		    	$k_range++;
		}
     }
  }
  else
  {
	# only if number of repair symbols values are defined
	if ( defined @{$params{"nb_repair_symbols"}})
	{
		@list_of_r = get_list_of_values(\@{$params{"nb_repair_symbols"}});
	}
      if (scalar @list_of_k != scalar @list_of_r)
      {
	print "Please, check your parameters file and correct nb_source_symbols and nb_repair_symbols parameters: they must have same length.\n";
	return -1;
      }
     @{$params{"nb_source_symbols"}} = @list_of_k;
     @{$params{"nb_repair_symbols"}} = @list_of_r;
  }
  $nb_tests = 1;
  print "Cleaning tmp directory...";
  `rm ./tmp/* >/dev/null 2>/dev/null`;
  print "Done.\n";

  print "Running tests...\r";
  # here, we start loops for tests.
  foreach $codec (@{$params{"codec"}})
  {  
    foreach $tx_type (@{$params{"tx_type"}})
    { 
	  foreach $ss (@{$params{"symbol_size"}})
	  {
	      $num_k = 0;
	      foreach $k (@{$params{"nb_source_symbols"}})
	      {
		$r = @{$params{"nb_repair_symbols"}}[$num_k];
		$num_k++;
		if ($codec >=3) #ldpc
		{
		 foreach $left_degree (@list_of_left_degrees) 
		 {
			   $tool_with_args = $tool." -k=".$k." -r=".$r." -codec=".$codec." -ldpc_N1=".$left_degree." -symb_sz=".$ss." -tx_type=".$tx_type;  
			   prepare_tests($tool_with_args,$iter,$nb_thread,\%params,$nb_tests);
		 }
		}
		else
		{
			if ($codec==2)
			{
				foreach my $m (@{$params{"rs_m"}})
				{
					$tool_with_args = $tool." -k=".$k." -r=".$r." -codec=".$codec." -rs_m=".$m." -symb_sz=".$ss." -tx_type=".$tx_type;  
					prepare_tests($tool_with_args,$iter,$nb_thread,\%params,$nb_tests);
				}				
			}
			else
			{
			   $tool_with_args = $tool." -k=".$k." -r=".$r." -codec=".$codec." -symb_sz=".$ss." -tx_type=".$tx_type;  
			   prepare_tests($tool_with_args,$iter,$nb_thread,\%params,$nb_tests);		
			}
		}
	      }
	 }
      }
    }


  #fill_database($dbh);
  
  $dbh->disconnect();

  print "Removing tmp files...\r";
  for (my $i=0;$i < $nb_tmp_files;$i++)
  {
    printf "Removing tmp files...%.2f %%\r", 100*$i/$nb_tmp_files;
    system("cat  ./tmp/tmp_$i.txt  >> $trace_file ");
    system("rm ./tmp/tmp_$i.txt");    
  }
  print "Removing tmp files...Done      \n";
}

sub prepare_tests
{
  my ($tool_with_args,$iter,$nb_thread,$params) = @_;
  my @threads;
  my $from=0;
  my $cmd;
  my $to=0; 
  my @tab_for_iter = get_nb_iters_for_each_threads($iter,$nb_thread);
  my @list_of_losses = get_list_of_losses(\@{$$params{"loss"}});
  
  if (  @{$$params{"using_ml"}}[0] eq "false")
  {
    foreach my $loss (@list_of_losses)
    {
      $from = 0;$to = 0;
      $cmd = $tool_with_args." -loss=".$loss;
      for (1..$nb_thread)
      {
	if($_ != 1)
	{
	    $from+=$tab_for_iter[$_-1];
	}
	if ($_ == $nb_thread)
	{
	  $to =$iter;
	}
	else
	{
	  $to += $tab_for_iter[$_];
	}
	$threads[$_] = threads->create("run_test2",($cmd,$from+(@{$$params{"offset_iteration"}}[0]),$to+(@{$$params{"offset_iteration"}}[0]),"./tmp/tmp_".$_.".txt",@{$$params{"nb_iterations_for_partial_results"}}[0],@{$$params{"using_ml"}}[0]));
      }
      for (1..$nb_thread)
      {
	$threads[$_]->join();
	system("cat ./tmp/tmp_".$_.".txt_backup >> $trace_file");
	unlink "./tmp/tmp_".$_.".txt_backup";
      }
    }
  }
  else
  {
    for (1..$nb_thread)
    {
      if($_ != 1)
      {
	  $from+=$tab_for_iter[$_-1];
      }
      if ($_ == $nb_thread)
      {
	$to =$iter;
      }
      else
      {
	$to += $tab_for_iter[$_];
      }
      $threads[$_] = threads->create("run_test2",($tool_with_args,$from,$to,"./tmp/tmp_".$_.".txt",@{$$params{"nb_iterations_for_partial_results"}}[0],@{$$params{"using_ml"}}[0]));
    }
    for (1..$nb_thread)
    {
      $threads[$_]->join();
      system("cat ./tmp/tmp_".$_.".txt_backup >> $trace_file");
      unlink "./tmp/tmp_".$_.".txt_backup";
    }
  }
}

sub get_nb_iters_for_each_threads
{
  my ($iter,$nb_threads) = @_;
  my $rest = $iter;
  my $grain = floor($iter/$nb_threads);
  my @ret;
  for (my $i=1;$i<$nb_threads;$i++)
  {
    push @ret,$grain;
    $rest -=$grain;
  }
  push @ret,$rest;
  return @ret;
}

sub get_list_of_values
{
  my @t;
  my $tab = shift;
  if ($$tab[2] < 1 or not defined $$tab[2]) { $$tab[2] =1 };
  for (my $i=$$tab[0];$i<=$$tab[1];$i+=$$tab[2])
  {
    push @t,$i;
  }
  return @t;
}

sub get_list_of_losses
{
  my @t;
  my $tab = shift;
  if ($$tab[3] < 1 or not defined $$tab[3]) { $$tab[3] =1 };
  for (my $i=$$tab[1];$i<=$$tab[2];$i+=$$tab[3])
  {
    push @t,"".$$tab[0].":".$i;
  }
  return @t;
}

sub fill_database_with_file(file)
{
  my $f = shift;
  $_=$connection_string[0];
  # are we using SQLite ?
  if (/SQLite/)
  {
	  $semaphore_sqlite->down();
  }
  my $dbh=DBI->connect(@connection_string);
  my %res;
  my $id_init_table=0;
  my $id_encoding_table;
  my $id_decoding_table;
  my $id_matrix_table;
  my $id_run_table;
  my $unik_id_run_table;
    open (I_FILE,$f) || die "Could not open $f\n";
    while (<I_FILE>) {
      SEARCH: {
		  /-loss/ && do {
		      $res{"loss_type"} = getval($_,'-loss');
		  };
		  /tot_nb_source_symbols/ && do {
		      $res{"nb_xor_it"} = 0;
		      $res{"nb_xor_ml"} = 0;
		      $res{"k"} = getval($_, 'tot_nb_source_symbols');
		      $res{"r"} = getval($_, 'tot_nb_repair_symbols');
		      $res{"ss"}= getval($_, 'symbol_size');
		      $res{"l"} = getval($_, 'ldpc_N1');
			  $res{"rs_m"} = getval($_, 'rs_m');
		  };
		  /codec_id/ && do {
		      $res{"c"} = getval($_,'codec_id');
		  };
		  /transmission_type/ && do {
		      $res{"tx"} = getval($_,'transmission_type');
		  };
		  /iter/ && do {
		    $res{"i"} = getval($_,'iter');
		  };
		  /random seed/ && do {
		    $res{"seed"} = getval($_,'seed');
		  };
		  /init_start/ && do {
		    $res{"init_start"} = getval($_,'init_start');
		  };
		  /init_end/ && do {
		    $res{"init_end"} = getval($_,'init_end');
		    $res{"init_time"} = getval($_,'init_time');
		  };
		  /encoding_start/ && do {
		    $res{"encoding_start"} = getval($_,'encoding_start');
		  };
		  /encoding_end/ && do {
		    $res{"encoding_end"} = getval($_,'encoding_end');
		    $res{"encoding_time"} = getval($_,'encoding_time');
		  };
		  /decoding_start/ && do {
		    $res{"decoding_start"} = getval($_,'decoding_start');
		  };
		  /nb_xor_for_IT/ && do {
		    $res{"nb_xor_it"} = getval($_,'nb_xor_for_IT');
	          };
		  /nb_xor_for_ML/ && do {
		    $res{"nb_xor_ml"} = getval($_,'nb_xor_for_ML');
	          };
		  /decoding_end/ && do {
		    $res{"decoding_end"} = getval($_,'decoding_end');
		    $res{"decoding_time"} = getval($_,'decoding_time');
		    $res{"nb_received_symbols"} = getval($_,'nb_received_symbols');
		    $res{"inef_ratio"} = getval($_,'inefficiency_ratio');
		  };

		  /decoding_status/ && do {
		      $res{"decoding_status"} = getval($_,'decoding_status');
		      
		    #remove space caracters
		    $res{"c"} =~ s/^\s+//;
			if (not $res{"decoding_status"} eq "0")
			{
				print "\n[ERROR] Ignoring this record..(-k=$res{\"k\"},-r=$res{\"r\"},-codec=$res{\"c\"},-loss =".get_loss_percentage($res{"loss_type"},$res{"k"},$res{"r"})."%)";
				next;
			}
			else
			{
				$dbh->do("INSERT INTO init_table (init_start,init_end,init_time) VALUES (".$res{"init_start"}.",".$res{"init_end"}.",".$res{"init_time"}.");") or die "database insert failed";
				$id_init_table = get_last_insert_id($dbh);
				$dbh->do("INSERT INTO encoding_table (encoding_start,encoding_end,encoding_time) VALUES (".$res{"encoding_start"}.",".$res{"encoding_end"}.",".$res{"encoding_time"}.");") or die "database insert failed";
				$id_encoding_table = get_last_insert_id($dbh);
				if ($res{"inef_ratio"} == "")
				{
				  $res{"nb_received_symbols"} = "0";
				  $dbh->do("INSERT INTO decoding_table (decoding_start,decoding_end,decoding_time,decoding_steps,decoding_inef,decoding_status) VALUES (".$res{"decoding_start"}.",0,0,0,0,".$res{"decoding_status"}.");") or die "database insert failed";
				}
				else
				{
				  $dbh->do("INSERT INTO decoding_table (decoding_start,decoding_end,decoding_time,decoding_steps,decoding_inef,decoding_status) VALUES (".$res{"decoding_start"}.",".$res{"decoding_end"}.",".$res{"decoding_time"}.",".$res{"nb_received_symbols"}.",".$res{"inef_ratio"}.",".$res{"decoding_status"}.");") or die "database insert failed";
				}
				$id_decoding_table = get_last_insert_id($dbh);
				if ($res{"c"} == '2') 
				{	
					$unik_id_run_table = get_run_id($dbh,$res{"k"},$res{"r"},$res{"c"},$res{"rs_m"},$res{"ss"},get_tx_id($dbh,$res{"tx"}));
				}
				else
				{
					$unik_id_run_table = get_run_id($dbh,$res{"k"},$res{"r"},$res{"c"},$res{"l"},$res{"ss"},get_tx_id($dbh,$res{"tx"}));
				}
				if ($unik_id_run_table == "")
				{
					if ($res{"c"} == '2') 
					{
						$dbh->do("INSERT INTO run_table (run_k,run_r,codec_id,run_left_degree,run_symbol_size,tx_id) VALUES (".
						$res{"k"}.",".$res{"r"}.",".$res{"c"}.",".$res{"rs_m"}.",".$res{"ss"}.",".get_tx_id($dbh,$res{"tx"}).");") or die "database insert failed";
					}
					else
					{
						$dbh->do("INSERT INTO run_table (run_k,run_r,codec_id,run_left_degree,run_symbol_size,tx_id) VALUES (".
						$res{"k"}.",".$res{"r"}.",".$res{"c"}.",".$res{"l"}.",".$res{"ss"}.",".get_tx_id($dbh,$res{"tx"}).");") or die "database insert failed";
					}
					$id_run_table = get_last_insert_id($dbh);
					$dbh->do("INSERT INTO iter_table (run_id,init_id,encoding_id,decoding_id,matrix_id,iter_num,iter_seed,iter_nb_received_symbols,iter_loss_percentage,iter_nb_xor_for_it,iter_nb_xor_for_ml) VALUES (".
					$id_run_table.",".$id_init_table.",".$id_encoding_table.",".$id_decoding_table.",0,".$res{"i"}.",".$res{"seed"}.",".$res{"nb_received_symbols"}.",".get_loss_percentage($res{"loss_type"},$res{"k"},$res{"r"}).",".$res{"nb_xor_it"}.",".$res{"nb_xor_ml"}.");") or die "database insert failed";		
				}
				else
				{
					$dbh->do("INSERT INTO iter_table (run_id,init_id,encoding_id,decoding_id,matrix_id,iter_num,iter_seed,iter_nb_received_symbols,iter_loss_percentage,iter_nb_xor_for_it,iter_nb_xor_for_ml) VALUES (".
					$unik_id_run_table.",".$id_init_table.",".$id_encoding_table.",".$id_decoding_table.",0,".$res{"i"}.",".$res{"seed"}.",".$res{"nb_received_symbols"}.",".get_loss_percentage($res{"loss_type"},$res{"k"},$res{"r"}).",".$res{"nb_xor_it"}.",".$res{"nb_xor_ml"}.");") or die "database insert failed";
				}
			}
		  };
	    }
  }
  close(I_FILE);
  $dbh->disconnect();
	$_=$connection_string[0];
	if (/SQLite/)
	{
		$semaphore_sqlite->up();
	}
}
 
# this is a test unit 
# it run the test for a group of iteration
# it will launch the main command with different seedss
sub find_max_nb_loss
{
    my $base_cmd = $_[0];
    my $max_nb_loss = $_[1];
    my $cmd ;
    my $result;
    my $success=0;
    my $decoding_success= -1;
    my $nb_loss;
    my $tmp;
    my $upper=$max_nb_loss;
    my $lower=0;
    no warnings;
    $nb_loss = $upper;
    #print "  max_nb_loss = $max_nb_loss\n";
    #print "   $lower  $upper    decoding_steps = $decoding_steps \n";
    while( floor($upper - $lower) > 1)
    {
      	#print "   $lower $nb_loss   $upper    =?decoding_steps = $decoding_steps \n";
	$nb_loss= floor(($upper + $lower)/2);
	#print "   $nb_loss  \n";
	$cmd = $base_cmd . ' -loss=3:'.$nb_loss;
	#print "cmd = $cmd \n";
	$result = `$cmd 2>&1`;
	#print " result = $result \n";
	#if( index($result, 'decoding_status') >= 0){
	   	$decoding_success= getval($result, 'decoding_status') ;
	#}
	if($decoding_success==0){
	    $lower = $nb_loss;
	}else{
	    $upper = $nb_loss;
	}
	    #print "   $lower $nb_loss   $upper    decoding_steps = $decoding_steps \n";
    }
    $nb_loss =  $lower;
    #print "nbloss=$nb_loss\n";
    $cmd = $base_cmd . ' -loss=3:'.($nb_loss);
    #print "$cmd\n";
    $result = `$cmd 2>&1`;
    $decoding_success= getval($result, 'decoding_status') ;      

	while ($decoding_success==0)
	{
	  $nb_loss++;
	  $cmd = $base_cmd . ' -loss=3:'.($nb_loss);
	  #print "$cmd\n";
	  $result = `$cmd 2>&1`;
	  $decoding_success= getval($result, 'decoding_status') ;      
	}

    #print "dichotomique research completed : nb_loss = $nb_loss\n";
    if ($nb_loss > 0) {
	# success, a value has been found for this seed. Take this value -1 so that decoding succeds
    	return --$nb_loss;
    } else {
	# failure, there is no possible solution. Return 0 which remains a valid loss value for eperftool
    	return 0;
    }
}

sub get_nb_max_loss_for(cmd)
{
  my $cmd = shift;
  my $k = getval($cmd,'-k');
  my $r = getval($cmd,'-r');
  return $k * ($r-1) + 1;
}

sub run_test2
{
  my ($cmd,$from,$to,$trc_file,$nb_iter_before_insert,$using_ml) = @_;
  my $max_nb_loss;
  my $i=1;
  my $iter;
  my $my_cmd;
  unlink $trc_file;
  for(my $iter=$from;$iter<$to;$iter++)
  {
    $my_cmd = $cmd ." -seed=".($iter+1);
    system("echo '\n\niter=$iter\n' >> $trc_file");
    if ($using_ml eq "true")
    {
      $max_nb_loss = find_max_nb_loss($my_cmd,&get_nb_max_loss_for($my_cmd));
      $my_cmd = $my_cmd . " -loss=3:" . $max_nb_loss;
    }
    system("echo $my_cmd >> $trc_file");
    system("$my_cmd >> $trc_file 2>&1");
    if ($i == $nb_iter_before_insert)
    {
      &fill_database_with_file($trc_file);
      system("cat $trc_file >> ".$trace_file."_backup");
      unlink $trc_file;
      $i=0;
    }
    $i++;
  }
  if($i !=1)
  {
    &fill_database_with_file($trc_file);
    system("cat $trc_file >> ".$trc_file."_backup");
    unlink $trc_file;
  }
return;
}

sub run_test
{
  my $trc_file = shift;
  my $nb_iter_before_insert = shift;
  my $using_ml = shift;
  my $max_nb_loss;
  my $i=1;
  my $cmd_and_iter;
  my $cmd;
  my $iter;
  unlink $trc_file;
  while (scalar @tab_cmd > 0)
  {
    $semaphore->down();
      $cmd_and_iter = shift @tab_cmd;
    $semaphore->up();
    ($cmd,$iter) = split("@",$cmd_and_iter);
    system("echo '\n\niter=$iter\n' >> $trc_file");
    system("echo $cmd >> $trc_file");
    if ($using_ml eq "true")
    {
      $max_nb_loss = find_max_nb_loss($cmd,&get_nb_max_loss_for($cmd));
      $cmd = $cmd . " -loss=3:" . $max_nb_loss;
    }
    system("$cmd >> $trc_file 2>&1");
    if ($i == $nb_iter_before_insert)
    {
      &fill_database_with_file($trc_file);
      system("cat $trc_file >> ".$trace_file."_backup");
      unlink $trc_file;
      $i=0;
    }
    $i++;
  }
  &fill_database_with_file($trc_file);
  system("cat $trc_file >> ".$trc_file."_backup");
  unlink $trc_file;
  return;
}

sub run_simple_test(cmd,trc_file,i,decoding_failure_probability_test)
{
  my $cmd = shift;
  my $trc_file = shift;
  my $iter = shift;
  my $using_ml = shift;
  my $max_nb_loss;
  
  system("echo '\n\niter=$iter\n' >> $trc_file");
  system("echo $cmd >> $trc_file");
    if ($using_ml eq "true")
    {
      $max_nb_loss = find_max_nb_loss($cmd,&get_nb_max_loss_for($cmd));
      $cmd = $cmd . " -loss=3:" . $max_nb_loss;
    }
    

  system("$cmd >> $trc_file 2>&1");

  return;
}

#only for mysql
sub erase_database(dbh)
{
  my $dbh = shift;
  my $database = shift;
  my @reqs= (
    "DROP TABLE IF EXISTS codec_table;",
    "DROP TABLE IF EXISTS encoding_table;",
    "DROP TABLE IF EXISTS decoding_table;",
    "DROP TABLE IF EXISTS init_table;",
    "DROP TABLE IF EXISTS tx_table;",
    "DROP TABLE IF EXISTS run_table;",
    "DROP TABLE IF EXISTS iter_table;",
    "DROP TABLE IF EXISTS matrix_table;");
  foreach  my $r (@reqs)
  {
    $dbh->do($r) or die "created failed";
  }
}

sub init_database(dbh)
{
  my $dbh=shift;
  my @reqs= (
    "INSERT INTO codec_table (codec_name) VALUES ('Reed-Solomon GF(2^8)');",
    "INSERT INTO codec_table (codec_name) VALUES ('Reed-Solomon GF(2^m)');",
    "INSERT INTO codec_table (codec_name) VALUES ('LDPC-Staircase');",
    "INSERT INTO codec_table (codec_name) VALUES ('not implemented');",
    "INSERT INTO codec_table (codec_name) VALUES ('2D parity matrix');",
    "INSERT INTO codec_table (codec_name) VALUES ('LDPC from file');",
    "INSERT INTO tx_table (tx_name) VALUES ('randomly_send_all_source_and_repair_symbols');",
    "INSERT INTO tx_table (tx_name) VALUES ('randomly_send_a_few_source_symbols_and_repair_symbols');",
    "INSERT INTO tx_table (tx_name) VALUES ('randomly_send_a_few_src_symbols_first_then_randomly_all_repair_symbols');",
    "INSERT INTO tx_table (tx_name) VALUES ('randomly_send_only_repair_symbols');",
    "INSERT INTO tx_table (tx_name) VALUES ('sequentially_send_all_src_symbols_first_then_repair_symbols');",
    "INSERT INTO tx_table (tx_name) VALUES ('sequentially_send_all_repair_symbols_first_then_src_symbols');",
    "INSERT INTO tx_table (tx_name) VALUES ('sequentially_send_all_src_symbols_first_then_randomly_src_symbols');",
    "INSERT INTO tx_table (tx_name) VALUES ('sequentially_send_all_repair_symbols_first_then_randomly_src_symbols');"
  );
  foreach  my $r (@reqs)
  {
    $dbh->do($r) or die "created failed";
  }
}


#create tables and default values ( particulary for codec table)
sub create_database_template(dbh,db_type)
{
  my $dbh = shift;
  my $db_type = shift;
  my @reqs;
  if ($db_type eq "mysql")
  {
    @reqs= (
    "CREATE TABLE IF NOT EXISTS codec_table ( codec_id INTEGER  AUTO_INCREMENT PRIMARY KEY, codec_name VARCHAR(50) );",
    "CREATE TABLE IF NOT EXISTS matrix_table (matrix_id INTEGER AUTO_INCREMENT PRIMARY KEY);",
    "CREATE TABLE IF NOT EXISTS tx_table (tx_id INTEGER AUTO_INCREMENT PRIMARY KEY, tx_name VARCHAR(100) );",
    "CREATE TABLE IF NOT EXISTS init_table ( init_id INTEGER  AUTO_INCREMENT PRIMARY KEY, init_start FLOAT, init_end FLOAT, init_time FLOAT );",
    "CREATE TABLE IF NOT EXISTS encoding_table ( encoding_id INTEGER  AUTO_INCREMENT PRIMARY KEY, encoding_start DOUBLE, encoding_end DOUBLE, encoding_time FLOAT );",
    "CREATE TABLE IF NOT EXISTS decoding_table ( decoding_id INTEGER  AUTO_INCREMENT PRIMARY KEY, decoding_start DOUBLE, decoding_end DOUBLE,decoding_time FLOAT, decoding_steps FLOAT, decoding_inef FLOAT,decoding_status BOOL );",
    "CREATE TABLE IF NOT EXISTS run_table ( run_id INTEGER  AUTO_INCREMENT PRIMARY KEY, run_k INTEGER, run_r INTEGER, codec_id INTEGER , run_left_degree INTEGER, run_symbol_size INTEGER,tx_id INTEGER,CONSTRAINT fk_codec_id FOREIGN KEY (codec_id) REFERENCES codec_table(codec_id),CONSTRAINT fk_tx_id FOREIGN KEY (tx_id) REFERENCES tx_table(tx_id));",
    "CREATE TABLE IF NOT EXISTS iter_table (iter_id INTEGER  AUTO_INCREMENT PRIMARY KEY, run_id INTEGER , 
    init_id INTEGER, 
    encoding_id INTEGER, 
    decoding_id INTEGER,
    matrix_id INTEGER,
    iter_num INTEGER, iter_seed INTEGER, iter_nb_received_symbols INTEGER, iter_loss_percentage FLOAT, iter_nb_xor_for_it INTEGER, iter_nb_xor_for_ml INTEGER,
    CONSTRAINT fk_run_id FOREIGN KEY (run_id) REFERENCES run_table(run_id),
    CONSTRAINT fk_init_id FOREIGN KEY (init_id) REFERENCES init_table(init_id),
    CONSTRAINT fk_encoding_id FOREIGN KEY (encoding_id) REFERENCES encoding_table(encoding_id),
    CONSTRAINT fk_decoding_id FOREIGN KEY (decoding_id) REFERENCES decoding_table(decoding_id),
    CONSTRAINT fk_matrix_id FOREIGN KEY (matrix_id) REFERENCES matrix_table(matrix_id));"
    );
  }
  else
  {
    @reqs= (
    "CREATE TABLE IF NOT EXISTS codec_table ( codec_id INTEGER PRIMARY KEY, codec_name VARCHAR(50) );",
    "CREATE TABLE IF NOT EXISTS matrix_table (matrix_id INTEGER PRIMARY KEY);",
    "CREATE TABLE IF NOT EXISTS tx_table (tx_id INTEGER PRIMARY KEY, tx_name VARCHAR(100) );",
    "CREATE TABLE IF NOT EXISTS init_table ( init_id INTEGER PRIMARY KEY, init_start FLOAT, init_end FLOAT, init_time FLOAT );",
    "CREATE TABLE IF NOT EXISTS encoding_table ( encoding_id INTEGER PRIMARY KEY, encoding_start DOUBLE, encoding_end DOUBLE, encoding_time FLOAT );",
    "CREATE TABLE IF NOT EXISTS decoding_table ( decoding_id INTEGER PRIMARY KEY, decoding_start DOUBLE, decoding_end DOUBLE,decoding_time FLOAT, decoding_steps FLOAT, decoding_inef FLOAT,decoding_status BOOL);",
    "CREATE TABLE IF NOT EXISTS run_table ( run_id INTEGER PRIMARY KEY, run_k INTEGER, run_r INTEGER, codec_id INTEGER CONSTRAINT fk_codec_id REFERENCES codec_table(codec_id), run_left_degree INTEGER, run_symbol_size INTEGER,tx_id INTEGER);",
    "CREATE TABLE IF NOT EXISTS iter_table (iter_id INTEGER PRIMARY KEY, run_id INTEGER CONSTRAINT fk_run_id REFERENCES run_table(run_id), init_id INTEGER CONSTRAINT fk_init_id REFERENCES init_table(init_id), encoding_id INTEGER CONSTRAINT fk_encoding_id REFERENCES encoding_table(encoding_id), decoding_id INTEGER CONSTRAINT fk_decoding_id REFERENCES decoding_table(decoding_id),matrix_id INTEGER CONSTRAINT fk_matrix_id REFERENCES matrix_table(matrix_id), iter_num INTEGER, iter_seed INTEGER,iter_nb_received_symbols INTEGER,iter_loss_percentage FLOAT,iter_nb_xor_for_it INTEGER,iter_nb_xor_for_ml INTEGER );"
    );
  }
  foreach  my $r (@reqs)
  {
    $dbh->do($r) or die "created failed";
  }
}

sub get_loss_percentage(str,k,r)
{
  my ($str,$k,$r) = @_;
  my @t = split(':',$str);
  if ($t[0] ==2) #percentage
  {
    return ($t[1]);
  }
  elsif($t[0] == 3) #number
  {
    return $t[1]/($k+$r)*100;
  }
  elsif($t[0] == 0)
  {
    return 0;
  }
}


#
# Search in record rec the string str and return the value
# that follows immediately "=".
#
sub getval(rec,str) {
	my	$rec = shift;
	my	$str = shift;
	my	@A;
	my	@B;

	$str = $str . '=';
	@A = split($str, $rec);
	die "ERROR, string \"$str\" not found in:\n$rec ($A[1])" unless ($#A >= 1);
	@B = split(/\s+/, $A[1]);	# split what follows $str using spaces
	return $B[0];			# and only consider the 1st field...
}

#read params in file
sub read_params(file,hash)
{
  my $file=shift;
  my $hash = shift;
  open(I_FILE,"<$file") || die "Could not open $file\n";
  while(<I_FILE>){ 
   next unless s/^(\w+)\s*//;
   $$hash{$1} = [split];
  }
  close(I_FILE);
}

#init params when a param isn't defined is the file
sub init_params(param)
{
  my $param = shift;
  $$param{"ldpc_N1"} = [5];
  $$param{"codec"} = [2];
  $$param{"nb_source_symbols"} = [1000];
  $$param{"nb_repair_symbols"} = [500];
  $$param{"symbol_size"} = [1024];
  $$param{"nb_iterations"} = [10];
  $$param{"nb_iterations_for_partial_results"} = [10000];
  $$param{"tx_type"}= [0];
  $$param{"loss"} = [0,0,0,1];
}

#usage
sub usage
{
  print<<EOM;
Usage : 
  $0
Performance test for openfec library.
Params : 
  -file_params	: the init file for running test

EOM
  exit(1);
}

sub get_last_insert_id(dbh)
{
  my $dbh=shift;
  my $id;
  my $prep;
  my $infos =  $dbh->get_info(  2 );
  if ($infos =~ /mysql/)
  { 
    $prep = $dbh->prepare("select last_insert_id();");
  }
  else
  {
    $prep = $dbh->prepare("select last_insert_rowid();");
  }
  
  $prep->execute() or die "echec";
  $id =  $prep->fetchrow_array;
  $prep->finish();
  return $id;
}


sub get_codec_id(dbh,codec_name)
{
  my $dbh = shift;
  my $codec_name=shift;
  my $id;
  my $prep = $dbh->prepare("select codec_id from codec_table where codec_name=?");
  $prep->execute($codec_name) or warn "unable to find codec_id";
  $id = $prep->fetchrow_array();
  $prep->finish();
  return $id;
}

sub get_tx_id(dbh,tx_name)
{
  my $dbh = shift;
  my $tx_name=shift;
  my $id;
  my $prep = $dbh->prepare("select tx_id from tx_table where tx_name=?");
  $prep->execute($tx_name) or warn "unable to find tx_id";
  $id = $prep->fetchrow_array();
  $prep->finish();
  return $id;
}

sub get_run_id(dbh,k,r,c,l,ss,tx)
{
  my ($dbh,$k,$r,$c,$l,$ss,$tx) = @_;

  my $id;

  my $prep = $dbh->prepare("select run_id from run_table where run_k=? and run_r=? and codec_id=? and run_left_degree=? and run_symbol_size=? and tx_id=?");
  $prep->execute($k,$r,$c,$l,$ss,$tx) or die "unable to find run_id";
  $id = $prep->fetchrow_array();
  $prep->finish();
  return $id;
}

sub  get_k_and_r_from_matrix_file()
{
    my $file = shift;
    my $k=-1;
    my $r=-1;
    my $n=-1;
    open(I_FILE,"<$file") || die "Could not open $file\n";
    $r=<I_FILE>;
    $n=<I_FILE>;
    $r=$r*1; # remove the space before the numerical value. Copyright Mathieu Cunche ;)
    $k=$n-$r;
    close(I_FILE);
    print "k=$k r=$r\n";
    return ($k,$r);
}

# get number of logical CPU depending of get_os function.
sub get_nb_cpu()
{
	my $os=get_os();
	SWITCH: for($os)
	{
		/linux/ && do { return `cat /proc/cpuinfo |grep "MHz"|wc -l`; last;};
		/darwin/ && do {my @t=split('=',`sysctl hw.availcpu`); return $t[1];last;};
		die "Unsupported platform.\n";
	}
}

#get the name of the current OS
sub get_os()
{
	return $^O;
}

