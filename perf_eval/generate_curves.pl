#!/usr/bin/perl
use strict;
use DBI;
use DBD::SQLite;
use Fraction;

#path to the parameters file given in argument
my $file_params=shift;
#check if argument is OK

help() unless $file_params;

my $start = time;
main();
my $end = time;
my $cpt=$end-$start;

print "\nGenerated in $cpt s.\n";
 
sub main
{

  # params hash
  my %params;
  #the database handle
  my $dbh;

  #descr stat tool
  my $descr_stat;
  my $tmp_file = "./tmp_file_".`date +%Y-%m-%d_%Hh%Mm%S`;
  chop($tmp_file);

  init_params(\%params);
  read_params($file_params,\%params);
  read_params_for_curve(\%params);
  my $database_type=  @{$params{"database"}}[0];

  if ($database_type eq "file")
  {
    #open a connexion to the database file
    $dbh = DBI->connect("dbi:SQLite:dbname=".@{$params{"database"}}[1],"","");
  }
  else
  {
    if ($database_type eq "server")
    {
  	#open a connexion to the database file
	$dbh = DBI->connect("dbi:mysql:database=".@{$params{"database"}}[1].";host=".@{$params{"database"}}[2].";port=".@{$params{"database"}}[3],@{$params{"database"}}[4],@{$params{"database"}}[5]);
    }
    else
    {
      print "Wrong database type. Exiting !\n";
      return;
    }
  }

  die "-curve parameter is not defined" if not defined $params{"curve"};
# switch for differents curves : 
#1 : decoding bitrate (Y axis) as a function of the loss percentage (X axis)
#2 : decoding failure probability (Y axis) as a function of the loss percentage(code_rate) (X axis)
#3 : decoding failure probability (Y axis) as a function of the number of received symbols(nb_source_symbol) (X axis)
#4 : inefficiency ratio (Y axis) as a function of the code rate (X axis)
#5 : inefficiency ratio (Y axis) as a function of the object size (X axis)
#6 : number of XOR operations (Y axis) as a function of the object size (X axis)
#7 : number of XOR operations (Y axis) as a function of the loss percentage (X axis)
  SWITCH: for ($params{"curve"})
  {
    /1/ && do { generate_decoding_bitrate_for_loss_percentage($dbh,\%params);  };
    /2/ && do { die "You have to specify the code_rate parameter." unless $params{"curve_code_rate"}; generate_decoding_failure_prob_for_loss_percentage($dbh,\%params,$params{"curve_code_rate"});};
    /3/ && do { die "You have to specify the nb_source_symbol parameter." unless $params{"curve_nb_source_symbol"}; generate_decoding_failure_prob_for_nb_received_symbols($dbh,\%params,$params{"curve_nb_source_symbol"} );};
    /4/ && do {generate_inef_ratio_for_code_rate($dbh,\%params);};
    /5/ && do { generate_inef_ratio_for_object_size($dbh,\%params); };
    /6/ && do { generate_nb_op_for_object_size($dbh,\%params);};
    /7/ && do {generate_nb_op_for_loss_percentage($dbh,\%params);};
      /8/ && do {generate_encoding_bitrate_for_loss_percentage($dbh,\%params); };
  }
}

sub get_table_and_col
{
  my $axis = shift;
  my $ret = "";
  SWITCH: for($axis){
    /loss_percentage/ && do { $ret="i.iter_loss_percentage"; last; };
     /nb_op/ && do { $ret="i.iter_nb_xor_for_it + i.iter_nb_xor_for_ml"; last; };
    /object_size/ && do { $ret="r.run_k"; last; };
    /decoding_bitrate/ && do { $ret="(8*r.run_symbol_size*r.run_k)/(avg(d.decoding_time)*1000*1000),
    (8*r.run_symbol_size*r.run_k)/(min(d.decoding_time)*1000*1000),
    (8*r.run_symbol_size*r.run_k)/(max(d.decoding_time)*1000*1000)"; last; };
  }
  return $ret;
}

sub generate_curves(dbh,params,y_axis,x_axis)
{
  my ($dbh,$params,$y_axis,$x_axis) = @_;
  my ($k,$symbol_size,$code_rate) = ("1000","4","2/3");
  my $request;
  my $readable_code_rate = $code_rate;
  $readable_code_rate =~ s/\//_/;
  my @res;
  my $sql_x_axis = get_table_and_col($x_axis);
  my $sql_y_axis = get_table_and_col($y_axis);

  my $file_cmd = $$params{"pref_1"}[0] . "test_".$y_axis."_on_".$x_axis."_k_" . $k . "_ss_". $symbol_size ."_code_rate_".$readable_code_rate."_". $$params{"pref_2"}[0] . ".dem";
  
  my $file_output = $$params{"pref_1"}[0] . "test_".$y_axis."_on_".$x_axis."_k_" . $k . "_ss_". $symbol_size ."_code_rate_".$readable_code_rate."_". $$params{"pref_2"}[0] . ".eps";  

  my @data_files;

  my (@codec_ids,@codec_names);
  get_codec_ids($dbh,\@codec_ids,\@codec_names);
  foreach(@codec_ids)
  {
    my $tmp_file_data = $$params{"pref_1"}[0] . "test_".$y_axis."_on_".$x_axis."_codec_" .$_ ."_k_". $k . "_ss_". $symbol_size ."_code_rate_".$readable_code_rate."_". $$params{"pref_2"}[0] . ".dat";
    $request = "SELECT ".$sql_y_axis.", ".$sql_x_axis."
    from run_table r,iter_table i, decoding_table d where ";
    if (not ($y_axis eq "object_size") and not ($x_axis  eq "object_size"))
    {
      $request = $request."r.run_k=$k and ";
    }
    $request = $request . "r.run_symbol_size = $symbol_size and
    abs(r.run_k/(r.run_k+r.run_r)) - $code_rate < 0.001 and
    r.codec_id=$_ and
	    i.decoding_id = d.decoding_id and r.run_id=i.run_id and d.decoding_status = 0
    group by ".$sql_x_axis." ORDER BY ".$sql_x_axis." ASC";
print $request;
    my $prep = $dbh->prepare($request);
    $prep->execute() or die "unable to find valid data";

    open(F_DAT,">$tmp_file_data") || die "Could not open $tmp_file_data\n";
    while (@res = $prep->fetchrow_array())
    {
	foreach (@res)
	{
	  print F_DAT "$_ ";
	}        
	print F_DAT "\n";
    }
    close(F_DAT);
    $prep->finish();
    push @data_files,$tmp_file_data;
  }

  open(F_DEM, ">$file_cmd") || die "Could not open $file_cmd\n";
  print F_DEM <<EOM;
set xlabel "loss percentage"
set ylabel "nb_op"
set autoscale
set nolabel
set nogrid
#set key box
set grid ytics mytics xtics
#set xrange [30:50]
#set size 0.75,0.75
#set yrange [0:1.1]
#set logscale y
set data style linespoints
EOM
	print F_DEM "set terminal postscript eps color\n";
	print F_DEM "set output \"".$file_output . "\"\n";
		my $title = "test ".$y_axis." for decoding";
		print F_DEM "plot ";
		for (my $i=0;$i<scalar @data_files;$i++)
		{
		  #print F_DEM "\"$data_files[$i]\" using 1:2:3:4 notitle  with errorbars lt ".($i+1)." ,";
		  print F_DEM "\"$data_files[$i]\" using 1:2 title \" $codec_names[$i]\" with lines lt ".($i+1);
		  if ($i < (scalar @data_files-1)) { printf F_DEM ","; }
		}
  close(F_DEM);  
}

sub generate_nb_op_for_loss_percentage(dbh,params)
{
  my ($dbh,$params) = @_;
  my @res;
  my $file_cmd = $$params{"pref_1"}[0] . "test_loss_percentage_". $$params{"pref_2"}[0] . ".dem";
  my $file_output = $$params{"pref_1"}[0] . "test_loss_percentage_". $$params{"pref_2"}[0] . ".eps";
  my @data_files;
  my @data_files_titles;

  my (@codec_ids,@codec_names);
  get_codec_ids($dbh,\@codec_ids,\@codec_names);
  foreach my $k (get_all_k($dbh))
  {
  	my @code_rates = get_list_of_code_rate_for_k($dbh,$k);
  	foreach my $cr (@code_rates)
  	{
	  foreach my $codec(@codec_ids)
	  {
	  	foreach my $ss (get_all_symbol_size($dbh))
	  	{
	  		if ($codec  >= 2)# ldpc codecs
	  		{
	  			my @N1;
	  			get_ldpc_N1($dbh,\@N1);
		  		foreach my $n1 (@N1)
		  		{
				    my $tmp_file_data;				    
				    my $tmp_title;
				    if ($codec ==2)
				    {
				        $tmp_file_data = $$params{"pref_1"}[0] . "test_loss_percentage_on_nb_op_rs_m_".$n1."_codec_" .$codec ."_k_". $k . "_ss_". $ss ."_code_rate_".$cr."_". $$params{"pref_2"}[0] . ".dat";
				     	$tmp_title = get_codec_name_for_codec_id($dbh,$codec). " (m=".$n1.") CR=".Math::Fraction::frac($cr)." K=".$k." Symb.S=".$ss;
				    }
				    else
				    {
				        $tmp_file_data = $$params{"pref_1"}[0] . "test_loss_percentage_on_nb_op_left_degree_".$n1."_codec_" .$codec ."_k_". $k . "_ss_". $ss ."_code_rate_".$cr."_". $$params{"pref_2"}[0] . ".dat";
				    	$tmp_title = get_codec_name_for_codec_id($dbh,$codec). " (N1=".$n1.") CR=".Math::Fraction::frac($cr)." K=".$k." Symb.S=".$ss;
				    }
				    my $r = $k/Math::Fraction::frac($cr) - $k;
				    my $prep = $dbh->prepare("SELECT i.iter_loss_percentage, avg(i.iter_nb_xor_for_it + i.iter_nb_xor_for_ml)
				    from run_table r,iter_table i, decoding_table d where 
				    r.run_k=? and
				    r.run_symbol_size = ? and
				    r.run_r = ? and
				    r.codec_id=? and
				    r.run_left_degree= ? and
				    i.decoding_id = d.decoding_id and r.run_id=i.run_id and d.decoding_status = 0
				    group by i.iter_loss_percentage ORDER BY i.iter_loss_percentage ASC");
				    $prep->execute($k,$ss, $r,$codec,$n1) or die "unable to find valid data";
				    open(F_DAT,">$tmp_file_data") || die "Could not open $tmp_file_data\n";
				    while (@res = $prep->fetchrow_array())
				    {
					foreach (@res)
					{
					  print F_DAT "$_ ";print "$_\n";
					}        
					print F_DAT "\n";
				    }
				    close(F_DAT);
				    $prep->finish();
				    push @data_files,$tmp_file_data;
				    push @data_files_titles,$tmp_title;
  				}
  			}
  			else
  			{
				    my $tmp_file_data = $$params{"pref_1"}[0] . "test_loss_percentage_on_nb_op_codec_" .$codec ."_k_". $k . "_ss_". $ss ."_code_rate_".$cr."_". $$params{"pref_2"}[0] . ".dat";
				    my $tmp_title = get_codec_name_for_codec_id($dbh,$codec). " CR=".Math::Fraction::frac($cr)." K=".$k." Symb.S=".$ss;   
				    my $r = $k/Math::Fraction::frac($cr) - $k; 
				    my $prep = $dbh->prepare("SELECT i.iter_loss_percentage, avg(i.iter_nb_xor_for_it + i.iter_nb_xor_for_ml)
				    from run_table r,iter_table i, decoding_table d where 
				    r.run_k=? and
				    r.run_symbol_size = ? and
				    r.run_r = ? and
				    r.codec_id=? and
					    i.decoding_id = d.decoding_id and r.run_id=i.run_id and d.decoding_status = 0
				    group by i.iter_loss_percentage ORDER BY i.iter_loss_percentage ASC");
				    $prep->execute($k,$ss, $r,$codec) or die "unable to find valid data";
				    open(F_DAT,">$tmp_file_data") || die "Could not open $tmp_file_data\n";
				    while (@res = $prep->fetchrow_array())
				    {
					foreach (@res)
					{
					  print F_DAT "$_ ";
					}        
					print F_DAT "\n";
				    }
				    close(F_DAT);
				    $prep->finish();
				    push @data_files,$tmp_file_data;
				    push @data_files_titles,$tmp_title;  			
  			}
  		}
  	}
      }
  }

  open(F_DEM, ">$file_cmd") || die "Could not open $file_cmd\n";
  print F_DEM <<EOM;
set xlabel "loss percentage"
set ylabel "nb_op"
set autoscale
set nolabel
set nogrid
#set key box
set grid ytics mytics xtics
#set xrange [30:50]
#set size 0.75,0.75
#set yrange [0:1.1]
#set logscale y
set data style linespoints
EOM
	print F_DEM "set terminal postscript eps color\n";
	print F_DEM "set output \"".$file_output . "\"\n";
		my $title = "test loss percentage for decoding";
		print F_DEM "plot ";
		for (my $i=0;$i<scalar @data_files;$i++)
		{
		  print F_DEM "\"$data_files[$i]\" using 1:2 title \" $data_files_titles[$i]\" with lines lt ".($i+1);
		  if ($i < (scalar @data_files-1)) { printf F_DEM ","; }
		}
  close(F_DEM);  
}

sub generate_nb_op_for_object_size(dbh,params)
{
  my ($dbh,$params) = @_;
  my @res;
  my $file_cmd    = $$params{"pref_1"}[0] . "test_object_size_". $$params{"pref_2"}[0] . ".dem";
  my $file_output = $$params{"pref_1"}[0] . "test_object_size_". $$params{"pref_2"}[0] . ".eps";
  my @data_files;
  my @data_files_titles;

  my (@codec_ids,@codec_names,@code_rates);
  get_codec_ids($dbh,\@codec_ids,\@codec_names);
  foreach my $cr (get_all_readable_code_rate($dbh,\@code_rates))
  {
  	 foreach my $codec (@codec_ids)
  	 {
	  	foreach my $ss (get_all_symbol_size($dbh))
	  	{
	  		if ($codec  >= 3)# ldpc codecs
	  		{
	  			my @N1;
	  			get_ldpc_N1($dbh,\@N1);
		  		foreach my $n1 (@N1)
		  		{
		  			my $tmp_title = get_codec_name_for_codec_id($dbh,$codec). " (N1=".$n1.") CR=".$cr." Symb.S=".$ss;
		  			$cr =~ s/\//_/;
					my $tmp_file_data = $$params{"pref_1"}[0] . "test_object_size_on_nb_op_codec_" .$codec . "_ss_". $ss ."_code_rate_".$cr."_". $$params{"pref_2"}[0] . ".dat";
				    my $prep = $dbh->prepare("SELECT r.run_k, avg(i.iter_nb_xor_for_it + i.iter_nb_xor_for_ml)
				    from run_table r,iter_table i, decoding_table d where 
				    r.run_symbol_size = ? and
				    r.run_left_degree=? and
				    r.run_k/(r.run_k+r.run_r) - $cr) < 0.001 and
				    r.codec_id=? and
					    i.decoding_id = d.decoding_id and r.run_id=i.run_id and d.decoding_status = 0
				    group by r.run_k  ORDER BY r.run_k ASC");
				    $prep->execute($ss,$n1, $codec) or die "unable to find valid data";
				    open(F_DAT,">$tmp_file_data") || die "Could not open $tmp_file_data\n";
				    while (@res = $prep->fetchrow_array())
				    {
					foreach (@res)
					{
					  print F_DAT "$_ ";
					}        
					print F_DAT "\n";
				    }
				    close(F_DAT);
				    $prep->finish();
				    push @data_files,$tmp_file_data;
				    push @data_files_titles,$tmp_title;
				  }
			  }
			  else
			  {
			  		my $tmp_title = get_codec_name_for_codec_id($dbh,$codec). " CR=".$cr." Symb.S=".$ss;
			  		$cr =~ s/\//_/;
					my $tmp_file_data = $$params{"pref_1"}[0] . "test_object_size_on_nb_op_codec_" .$codec . "_ss_". $ss ."_code_rate_".$cr."_". $$params{"pref_2"}[0] . ".dat";					
				    my $prep = $dbh->prepare("SELECT r.run_k, avg(i.iter_nb_xor_for_it + i.iter_nb_xor_for_ml)
				    from run_table r,iter_table i, decoding_table d where 
				    r.run_symbol_size = ? and
				    abs(r.run_k/(r.run_k+r.run_r) - $cr) < 0.001 and
				    r.codec_id=? and
					    i.decoding_id = d.decoding_id and r.run_id=i.run_id and d.decoding_status = 0
				    group by r.run_k  ORDER BY r.run_k ASC");
				    $prep->execute($ss,$codec) or die "unable to find valid data";
				    open(F_DAT,">$tmp_file_data") || die "Could not open $tmp_file_data\n";
				    while (@res = $prep->fetchrow_array())
				    {
					foreach (@res)
					{
					  print F_DAT "$_ ";
					}        
					print F_DAT "\n";
				    }
				    close(F_DAT);
				    $prep->finish();
				    push @data_files,$tmp_file_data;
				    push @data_files_titles,$tmp_title;			  	
			  }
		}
	}
  }
			  

  open(F_DEM, ">$file_cmd") || die "Could not open $file_cmd\n";
  print F_DEM <<EOM;
set xlabel "Number of source symbol"
set ylabel "Number of symbol operations"
set autoscale
set nolabel
set nogrid
#set key box
set grid ytics mytics xtics
#set xrange [30:50]
#set size 0.75,0.75
#set yrange [0:1.1]
#set logscale y
set data style linespoints
EOM
	print F_DEM "set terminal postscript eps color\n";
	print F_DEM "set output \"".$file_output . "\"\n";
		my $title = "test loss percentage for decoding";
		print F_DEM "plot ";
		for (my $i=0;$i<scalar @data_files;$i++)
		{
		  #print F_DEM "\"$data_files[$i]\" using 1:2:3:4 notitle  with errorbars lt ".($i+1)." ,";
		  print F_DEM "\"$data_files[$i]\" using 1:2 title \" $data_files_titles[$i]\" with lines lt ".($i+1);
		  if ($i < (scalar @data_files-1)) { printf F_DEM ","; }
		}
  close(F_DEM);  
}

sub generate_encoding_bitrate_for_loss_percentage(dbh,params)
{
    my ($dbh,$params) = @_;
    my @res;
    my $file_cmd = $$params{"pref_1"}[0] . "test_encoding_loss_percentage_". $$params{"pref_2"}[0] . ".dem";
    my $file_output = $$params{"pref_1"}[0] . "test_encoding_loss_percentage_". $$params{"pref_2"}[0] . ".eps";
    my @data_files;
    my @data_files_titles;
    
    my (@codec_ids,@codec_names);
    get_codec_ids($dbh,\@codec_ids,\@codec_names);
    foreach my $k (get_all_k($dbh))
    {
        print "k=$k\n";
        my @code_rates = get_list_of_code_rate_for_k($dbh,$k);
        foreach my $cr (@code_rates)
        {
            print "cr=$cr\n";
            foreach my $codec(@codec_ids)
            {
                print "codec=$codec\n";
                foreach my $ss (get_all_symbol_size($dbh))
                {
                    if ($codec  >= 2)# ldpc codecs
                    {
                        my @N1;
                        get_ldpc_N1($dbh,\@N1);
                        foreach my $n1 (@N1)
                        {
                            my $tmp_file_data;				    
                            my $tmp_title;
                            if ($codec ==2)
                            {
                                $tmp_file_data = $$params{"pref_1"}[0] . "test_loss_percentage_on_encoding_bitrate_rs_m_".$n1."_codec_" .$codec ."_k_". $k . "_ss_". $ss ."_code_rate_".$cr."_". $$params{"pref_2"}[0] . ".dat";
                                $tmp_title = get_codec_name_for_codec_id($dbh,$codec). " (m=".$n1.") CR=".Math::Fraction::frac($cr)." K=".$k." Symb.S=".$ss;
                            }
                            else
                            {
                                $tmp_file_data = $$params{"pref_1"}[0] . "test_loss_percentage_on_encoding_bitrate_left_degree_".$n1."_codec_" .$codec ."_k_". $k . "_ss_". $ss ."_code_rate_".$cr."_". $$params{"pref_2"}[0] . ".dat";
                                $tmp_title = get_codec_name_for_codec_id($dbh,$codec). " (N1=".$n1.") CR=".Math::Fraction::frac($cr)." K=".$k." Symb.S=".$ss;
                            }
				my $r = $k / Math::Fraction::frac($cr)  - $k;				    			    
                            my $prep = $dbh->prepare("select i.iter_loss_percentage, (8*r.run_symbol_size*r.run_k)/(avg(d.encoding_time)*1000*1000),
                            (8*r.run_symbol_size*r.run_k)/(min(d.encoding_time)*1000*1000),
                            (8*r.run_symbol_size*r.run_k)/(max(d.encoding_time)*1000*1000) from 
                            run_table r,iter_table i, encoding_table d where 
                            r.run_k=? and
                            r.run_left_degree=? and
                            r.run_symbol_size = ? and
                            r.run_r = ? and
                            r.codec_id=? and
                            i.encoding_id = d.encoding_id and r.run_id=i.run_id 
                            group by i.iter_loss_percentage ORDER BY i.iter_loss_percentage ASC");
                            $prep->execute($k,$n1,$ss, $r,$codec) or die "unable to find valid data";
                            open(F_DAT,">$tmp_file_data") || die "Could not open $tmp_file_data\n";
                            while (@res = $prep->fetchrow_array())
                            {
                                foreach (@res)
                                {
                                    print F_DAT "$_ ";
                                }        
                                print F_DAT "\n";
                            }
                            close(F_DAT);
                            $prep->finish();
                            push @data_files,$tmp_file_data;
                            push @data_files_titles,$tmp_title;
                        }
                    }
                    else # not ldpc => not N1
                    {
                        my $tmp_file_data = $$params{"pref_1"}[0] . "test_loss_percentage_on_encoding_bitrate_codec_" .$codec ."_k_". $k . "_ss_". $ss ."_code_rate_".$cr."_". $$params{"pref_2"}[0] . ".dat";
                        my $tmp_title = get_codec_name_for_codec_id($dbh,$codec). " CR=".Math::Fraction::frac($cr)." K=".$k." Symb.S=".$ss;
                        my $r = $k / Math::Fraction::frac($cr) - $k;
			my $prep = $dbh->prepare("select i.iter_loss_percentage, (8*r.run_symbol_size*r.run_k)/(avg(d.encoding_time)*1000*1000),
                        (8*r.run_symbol_size*r.run_k)/(min(d.encoding_time)*1000*1000),
                        (8*r.run_symbol_size*r.run_k)/(max(d.encoding_time)*1000*1000) from 
                        run_table r,iter_table i, encoding_table d where 
                        r.run_k=? and
                        r.run_symbol_size = ? and
                        r.run_r = ? and
                        r.codec_id=? and
                        i.encoding_id = d.encoding_id and r.run_id=i.run_id
                        group by i.iter_loss_percentage ORDER BY i.iter_loss_percentage ASC");
                        $prep->execute($k,$ss, $cr,$codec) or die "unable to find valid data";
                        open(F_DAT,">$tmp_file_data") || die "Could not open $tmp_file_data\n";
                        while (@res = $prep->fetchrow_array())
                        {
                            foreach (@res)
                            {
                                print F_DAT "$_ ";
                            }        
                            print F_DAT "\n";
                        }
                        close(F_DAT);
                        $prep->finish();
                        push @data_files,$tmp_file_data;	
                        push @data_files_titles,$tmp_title;	 	
                    }
                }
            }
        }
    }
    
    open(F_DEM, ">$file_cmd") || die "Could not open $file_cmd\n";
    print F_DEM <<EOM;
set xlabel "loss percentage"
set ylabel "min/aver/max Bitrate (Mbps)"
set autoscale
set nolabel
set nogrid
    #set key box
set grid ytics mytics xtics
    #set xrange [30:50]
    #set size 0.75,0.75
    #set yrange [0:1.1]
    #set logscale y
set data style linespoints
EOM
	print F_DEM "set terminal postscript eps color\n";
	print F_DEM "set output \"".$file_output . "\"\n";
    my $title = "test loss percentage for encoding";
    print F_DEM "plot ";
    for (my $i=0;$i<scalar @data_files;$i++)
    {
        print F_DEM "\"$data_files[$i]\" using 1:2:3:4 notitle  with errorbars lt ".($i+1)." ,";
        print F_DEM "\"$data_files[$i]\" using 1:2 title \" $data_files_titles[$i]\" with lines lt ".($i+1);
        if ($i < (scalar @data_files-1)) { printf F_DEM ","; }
    }
    close(F_DEM);      
}


sub generate_decoding_bitrate_for_loss_percentage(dbh,params)
{
  my ($dbh,$params) = @_;
  my @res;
  my $file_cmd = $$params{"pref_1"}[0] . "test_loss_percentage_". $$params{"pref_2"}[0] . ".dem";
  my $file_output = $$params{"pref_1"}[0] . "test_loss_percentage_". $$params{"pref_2"}[0] . ".eps";
  my @data_files;
  my @data_files_titles;

  my (@codec_ids,@codec_names);
  get_codec_ids($dbh,\@codec_ids,\@codec_names);
  foreach my $k (get_all_k($dbh))
  {
  	print "k=$k\n";
  	my @code_rates = get_list_of_code_rate_for_k($dbh,$k);
  	foreach my $cr (@code_rates)
  	{
  	print "cr=$cr\n";
	  foreach my $codec(@codec_ids)
	  {
	  	print "codec=$codec\n";
	  	foreach my $ss (get_all_symbol_size($dbh))
	  	{
	  		if ($codec  >= 2)# ldpc codecs
	  		{
	  			my @N1;
	  			get_ldpc_N1($dbh,\@N1);
		  		foreach my $n1 (@N1)
		  		{
				    my $tmp_file_data;				    
				    my $tmp_title;
				    if ($codec ==2)
				    {
				        $tmp_file_data = $$params{"pref_1"}[0] . "test_loss_percentage_on_bitrate_rs_m_".$n1."_codec_" .$codec ."_k_". $k . "_ss_". $ss ."_code_rate_".$cr."_". $$params{"pref_2"}[0] . ".dat";
				     	$tmp_title = get_codec_name_for_codec_id($dbh,$codec). " (m=".$n1.") CR=".Math::Fraction::frac($cr)." K=".$k." Symb.S=".$ss;
				    }
				    else
				    {
				        $tmp_file_data = $$params{"pref_1"}[0] . "test_loss_percentage_on_bitrate_left_degree_".$n1."_codec_" .$codec ."_k_". $k . "_ss_". $ss ."_code_rate_".$cr."_". $$params{"pref_2"}[0] . ".dat";
				    	$tmp_title = get_codec_name_for_codec_id($dbh,$codec). " (N1=".$n1.") CR=".Math::Fraction::frac($cr)." K=".$k." Symb.S=".$ss;
				    }				    			    
				    my $r = $k / Math::Fraction::frac($cr) - $k;
					print "r=$r,cr=$cr\n";
				    my $prep = $dbh->prepare("select i.iter_loss_percentage, (8*r.run_symbol_size*r.run_k)/(avg(d.decoding_time)*1000*1000),
				    (8*r.run_symbol_size*r.run_k)/(min(d.decoding_time)*1000*1000),
				    (8*r.run_symbol_size*r.run_k)/(max(d.decoding_time)*1000*1000) from 
					    run_table r,iter_table i, decoding_table d where 
				    r.run_k=? and
				    r.run_left_degree=? and
				    r.run_symbol_size = ? and
				    r.run_r = ? and
				    r.codec_id=? and
					    i.decoding_id = d.decoding_id and r.run_id=i.run_id and d.decoding_status = 0
				    group by i.iter_loss_percentage ORDER BY i.iter_loss_percentage ASC");
				    $prep->execute($k,$n1,$ss, $r,$codec) or die "unable to find valid data";
				    open(F_DAT,">$tmp_file_data") || die "Could not open $tmp_file_data\n";
				    while (@res = $prep->fetchrow_array())
				    {
					foreach (@res)
					{
					  print F_DAT "$_ ";
					}        
					print F_DAT "\n";
				    }
				    close(F_DAT);
				    $prep->finish();
				    push @data_files,$tmp_file_data;
				    push @data_files_titles,$tmp_title;
			 	}
		 	}
		 	else # not ldpc => not N1
		 	{
			    my $tmp_file_data = $$params{"pref_1"}[0] . "test_loss_percentage_on_bitrate_codec_" .$codec ."_k_". $k . "_ss_". $ss ."_code_rate_".$cr."_". $$params{"pref_2"}[0] . ".dat";
			    my $tmp_title = get_codec_name_for_codec_id($dbh,$codec). " CR=".Math::Fraction::frac($cr)." K=".$k." Symb.S=".$ss;
			    my $r = $k / Math::Fraction::frac($cr) - $k;			    
			    my $prep = $dbh->prepare("select i.iter_loss_percentage, (8*r.run_symbol_size*r.run_k)/(avg(d.decoding_time)*1000*1000),
			    (8*r.run_symbol_size*r.run_k)/(min(d.decoding_time)*1000*1000),
			    (8*r.run_symbol_size*r.run_k)/(max(d.decoding_time)*1000*1000) from 
				    run_table r,iter_table i, decoding_table d where 
			    r.run_k=? and
			    r.run_symbol_size = ? and
			    r.run_r = ? and
			    r.codec_id=? and
				    i.decoding_id = d.decoding_id and r.run_id=i.run_id and d.decoding_status = 0
			    group by i.iter_loss_percentage ORDER BY i.iter_loss_percentage ASC");
			    $prep->execute($k,$ss, $r,$codec) or die "unable to find valid data";
			    open(F_DAT,">$tmp_file_data") || die "Could not open $tmp_file_data\n";
			    while (@res = $prep->fetchrow_array())
			    {
				foreach (@res)
				{
				  print F_DAT "$_ ";
				}        
				print F_DAT "\n";
			    }
			    close(F_DAT);
			    $prep->finish();
			    push @data_files,$tmp_file_data;	
			    push @data_files_titles,$tmp_title;	 	
		 	}
	 	}
	  }
         }
  }

  open(F_DEM, ">$file_cmd") || die "Could not open $file_cmd\n";
  print F_DEM <<EOM;
set xlabel "loss percentage"
set ylabel "min/aver/max Bitrate (Mbps)"
set autoscale
set nolabel
set nogrid
#set key box
set grid ytics mytics xtics
#set xrange [30:50]
#set size 0.75,0.75
#set yrange [0:1.1]
#set logscale y
set data style linespoints
EOM
	print F_DEM "set terminal postscript eps color\n";
	print F_DEM "set output \"".$file_output . "\"\n";
		my $title = "test loss percentage for decoding";
		print F_DEM "plot ";
		for (my $i=0;$i<scalar @data_files;$i++)
		{
		  print F_DEM "\"$data_files[$i]\" using 1:2:3:4 notitle  with errorbars lt ".($i+1)." ,";
		  print F_DEM "\"$data_files[$i]\" using 1:2 title \" $data_files_titles[$i]\" with lines lt ".($i+1);
		  if ($i < (scalar @data_files-1)) { printf F_DEM ","; }
		}
  close(F_DEM);  
}

#TODO : fix ratio, codec,symbol_size and left_degree
sub generate_enc_and_dec_time_for_object_size(dbh,params)
{
  my ($dbh,$params) = (shift,shift);
  my $nb_cols = 7;
  my @res;
  my $file_cmd = $$params{"pref_1"}[0][0] . "test_k_impacts_on_enc_and_dec" . $$params{"pref_2"}[0][0] . ".dem";
  my $file_data = $$params{"pref_1"}[0][0] . "test_k_impacts_on_enc_and_dec" . $$params{"pref_2"}[0][0] . ".dat";
  my $file_output = $$params{"pref_1"}[0][0] . "test_k_impacts_on_enc_and_dec" . $$params{"pref_2"}[0][0] . ".eps";

  my $prep = $dbh->prepare("select r.run_k, avg(e.encoding_time),min(e.encoding_time),max(e.encoding_time), 
	avg(d.decoding_time),min(d.decoding_time),max(d.decoding_time) from 
	run_table r,iter_table i, decoding_table d,encoding_table e where 
	i.encoding_id = e.encoding_id and i.decoding_id = d.decoding_id and r.run_id=i.run_id group by i.run_id 
	order by r.run_k;");
  $prep->execute() or die "unable to find valid data";
  open(F_DAT,">$file_data") || die "Could not open $file_data\n";
  while (@res = $prep->fetchrow_array())
  {
      
      foreach (@res)
      {
	print F_DAT "$_ ";
      }        
      print F_DAT "\n";
  }
  close(F_DAT);
  $prep->finish();


  open(F_DEM, ">$file_cmd") || die "Could not open $file_cmd\n";
  print F_DEM <<EOM;
set xlabel "k (in nb of symbols)"
set ylabel "min/aver/max enc or dec (in s)"
set autoscale
set nolabel
set nogrid
#set key box
set grid ytics mytics xtics
#set xrange [30:50]
#set size 0.75,0.75
#set yrange [0:1.1]
#set logscale y
set data style linespoints
EOM
	print F_DEM "set terminal postscript eps color\n";
	print F_DEM "set output \"".$file_output . "\"\n";
		my $title = "test k impacts for encoding";
		 my $title2 = "test k impacts for decoding";
		print F_DEM "plot \"$file_data\" using 1:2:3:4 notitle  with errorbars lt 1 ,";
		print F_DEM "\"$file_data\" using 1:5:6:7 notitle  with errorbars lt 3,";
		print F_DEM "\"$file_data\" using 1:2 title \" $title\" with lines lt 1,";
		print F_DEM "\"$file_data\" using 1:5 title \" $title2\" with lines lt 3";
  close(F_DEM);
}

sub generate_decoding_failure_prob_for_loss_percentage(dbh,params,code_rate)
{
  my ($dbh,$params,$cr) = @_;
  my $loss_percentage = 100-$cr*100;

  my $nb_occ;
  my @res;
  my $file_cmd = $$params{"pref_1"}[0] . "test_loss_percentage_". $$params{"pref_2"}[0] . ".dem";
  my $file_output = $$params{"pref_1"}[0] . "test_loss_percentage_". $$params{"pref_2"}[0] . ".eps";
  my @data_files;
  my @data_files_titles;
  my $max_nb_iter=0;
 
   my $readable_code_rate = $cr;
   $readable_code_rate =~ s/\//_/;

  my (@codec_ids,@codec_names);
  get_codec_ids($dbh,\@codec_ids,\@codec_names);
  
  foreach my $k (get_list_of_k_for_code_rate($dbh,$cr))
  {
  	foreach my $codec (@codec_ids)
  	{
  		foreach my $ss (get_all_symbol_size($dbh))
		{
	  		if ($codec  >= 2)# ldpc codecs
	  		{
	  			my @N1;
	  			get_ldpc_N1($dbh,\@N1);
		  		foreach my $n1 (@N1)
		  		{
					    my @tab_nb_received_symbols;
					    my @tab_decoding_success;
					    my $tmp_file_data;
					    my $tmp_title;
					    if ($codec==2)
					    {
					       $tmp_file_data = $$params{"pref_1"}[0] . "test_loss_percentage_on_decoding_failure_codec_" .$codec ."_rs_m_".$n1."_k_". $k . "_ss_". $ss ."_code_rate_".$readable_code_rate."_". $$params{"pref_2"}[0] . ".dat";
					       $tmp_title = get_codec_name_for_codec_id($dbh,$codec). " (m=".$n1.") CR=".Math::Fraction::frac($cr)." K=".$k." Symb.S=".$ss;
					    }
					    else
					    {
					       $tmp_file_data = $$params{"pref_1"}[0] . "test_loss_percentage_on_decoding_failure_codec_" .$codec ."_left_degree_".$n1."_k_". $k . "_ss_". $ss ."_code_rate_".$readable_code_rate."_". $$params{"pref_2"}[0] . ".dat";
					       $tmp_title = get_codec_name_for_codec_id($dbh,$codec). " (N1=".$n1.") CR=".Math::Fraction::frac($cr)." K=".$k." Symb.S=".$ss;					    
					    }

					    my $nb_iter = get_nb_iter_for_params_with_n1($dbh,$k,$n1,$ss,$cr,$codec);
					    if ($nb_iter > $max_nb_iter)
					    {
						    $max_nb_iter = $nb_iter;
					    }
						my $r = $k / Math::Fraction::frac($cr) - $k;
					    my $prep = $dbh->prepare("select count(d.decoding_status),100*round(1-round(avg(i.iter_nb_received_symbols)/round(r.run_k+r.run_r,9),9),9) from run_table r,decoding_table d, iter_table i, codec_table c
					 where d.decoding_id = i.decoding_id and d.decoding_status = 0 and 
					r.run_k=? and 
					r.run_left_degree=? and
					r.run_id=i.run_id and 
					c.codec_id = r.codec_id and 
					r.codec_id = ? and 
					r.run_symbol_size = ?
					and r.run_r = ? 
					group by i.iter_nb_received_symbols");
					    $prep->execute($k,$n1,$codec,$ss,$r) or die "unable to find valid data";
					    while (@res = $prep->fetchrow_array())
					    {
						push @tab_decoding_success , $res[0]; 
						push @tab_nb_received_symbols,$res[1];
					    }
					    for (my $i=(scalar @tab_decoding_success -2);$i>=0;$i--)
					    {
					      $tab_decoding_success[$i] +=$tab_decoding_success[$i+1];
					    }
					    
					    open(F_DAT,">$tmp_file_data") || die "Could not open $tmp_file_data\n";
					    for (my $i=0;$i <scalar @tab_decoding_success-1;$i++)
					    {
						print F_DAT "".($tab_nb_received_symbols[$i])." ".($tab_decoding_success[$i]/$max_nb_iter);
						print F_DAT "\n";
					    }
					    close(F_DAT);
					    $prep->finish();
					    push @data_files,$tmp_file_data;
					    push @data_files_titles,$tmp_title;
				}
			}
			else
			{
					    my @tab_nb_received_symbols;
					    my @tab_decoding_success;
					    my $tmp_file_data = $$params{"pref_1"}[0] . "test_loss_percentage_on_decoding_failure_codec_" .$codec ."_k_". $k . "_ss_". $ss ."_code_rate_".$readable_code_rate."_". $$params{"pref_2"}[0] . ".dat";
					    my $tmp_title = get_codec_name_for_codec_id($dbh,$codec). " CR=".Math::Fraction::frac($cr)." K=".$k." Symb.S=".$ss;
					    my $nb_iter = get_nb_iter_for_params_without_n1($dbh,$k,$ss,$cr,$codec);
					    if ($nb_iter > $max_nb_iter)
					    {
						    $max_nb_iter = $nb_iter;
					    }				
					    my $r = $k/Math::Fraction::frac($cr)-$k;	    
					    my $prep = $dbh->prepare("select count(d.decoding_status),100*round(1-round(avg(i.iter_nb_received_symbols)/round(r.run_k+r.run_r,9),9),9) from run_table r,decoding_table d, iter_table i, codec_table c
					 where d.decoding_id = i.decoding_id and d.decoding_status = 0 and 
					r.run_k=? and 
					r.run_id=i.run_id and 
					c.codec_id = r.codec_id and 
					r.codec_id = ? and 
					r.run_symbol_size = ?
					and r.run_r = ? 
					group by i.iter_nb_received_symbols");
					    $prep->execute($k,$codec,$ss,$r) or die "unable to find valid data";
					    while (@res = $prep->fetchrow_array())
					    {
						push @tab_decoding_success , $res[0]; 
						push @tab_nb_received_symbols,$res[1];
					    }
					    for (my $i=(scalar @tab_decoding_success -2);$i>=0;$i--)
					    {
					      $tab_decoding_success[$i] +=$tab_decoding_success[$i+1];
					    }
					    
					    open(F_DAT,">$tmp_file_data") || die "Could not open $tmp_file_data\n";
					    for (my $i=0;$i <scalar @tab_decoding_success-1;$i++)
					    {
						print F_DAT "".($tab_nb_received_symbols[$i])." ".($tab_decoding_success[$i]/$max_nb_iter);
						print F_DAT "\n";
					    }
					    close(F_DAT);
					    $prep->finish();
					    push @data_files,$tmp_file_data;
					    push @data_files_titles,$tmp_title;				
			}
		}
	}
  }

  open(F_DEM, ">$file_cmd") || die "Could not open $file_cmd\n";
  print F_DEM <<EOM;
set xlabel "loss percentage"
set ylabel "decoding failure probabilitiy"
set autoscale
set nolabel
set nogrid
#set key box
set grid ytics mytics xtics
set size 0.75,0.75
set logscale y
set data style linespoints
EOM
  print F_DEM "set arrow from  $loss_percentage,".1/$max_nb_iter." to $loss_percentage,1  nohead lt -1\n";
  print F_DEM "set xrange [".($loss_percentage-15).":".($loss_percentage+5)."]\n";
  print F_DEM "set yrange [".1/$max_nb_iter.":1.5]\n";
  print F_DEM "set terminal postscript eps color\n";
  print F_DEM "set output \"".$file_output . "\"\n";
  print F_DEM "plot ";
  for (my $i=0;$i<scalar @data_files;$i++)
  {
    print F_DEM "\"$data_files[$i]\" using 1:2 title \" $data_files_titles[$i] \"  with lines lt ".($i+1);
    if ($i < (scalar @data_files-1)) { printf F_DEM ","; }
  }
  close(F_DEM); 
}

sub generate_decoding_failure_prob_for_nb_received_symbols(dbh,params,k)
{
  my ($dbh,$params,$k) = @_;

  my $nb_occ;
  my @res;
  my $file_cmd = $$params{"pref_1"}[0] . "test_nb_received_symbols_". $$params{"pref_2"}[0] . ".dem";
  my $file_output = $$params{"pref_1"}[0] . "test_nb_received_symbols_". $$params{"pref_2"}[0] . ".eps";
  my @data_files;
  my @data_files_titles;
  my $max_nb_iter=0;
  my @nb_samples;
  my $total_nb_samples=0;
  foreach my $cr (get_list_of_code_rate_for_k($dbh,$k))
  {
	  my (@codec_ids,@codec_names);
	  get_codec_ids($dbh,\@codec_ids,\@codec_names);
	  foreach my $codec (@codec_ids)
	  {
	  	foreach my $ss (get_all_symbol_size($dbh))
	  	{
	  	  
	  		if ($codec  >= 2)# ldpc codecs
	  		{
	  			my @N1;
	  			get_ldpc_N1($dbh,\@N1);
		  		foreach my $n1 (@N1)
		  		{	  	
					    my @tab_nb_received_symbols;
					    my @tab_decoding_success;
					    my @tab_nb_samples;
					    my $tmp_file_data;
					    my $tmp_title;
					    if ($codec==2)
					    {
					    	$tmp_file_data = $$params{"pref_1"}[0] . "test_nb_received_symbols_on_decoding_failure_rs_m_".$n1."_codec_" .$codec. "_k_". $k . "_ss_". $ss ."_code_rate_".$cr."_". $$params{"pref_2"}[0] . ".dat";
					    	$tmp_title = get_codec_name_for_codec_id($dbh,$codec). " (m=".$n1.") CR=".Math::Fraction::frac($cr)." K=".$k." Symb.S=".$ss;					        
					    }else{
					    	$tmp_file_data = $$params{"pref_1"}[0] . "test_nb_received_symbols_on_decoding_failure_left_degree_".$n1."_codec_" .$codec. "_k_". $k . "_ss_". $ss ."_code_rate_".$cr."_". $$params{"pref_2"}[0] . ".dat";
					    	$tmp_title = get_codec_name_for_codec_id($dbh,$codec). " (N1=".$n1.") CR=".Math::Fraction::frac($cr)." K=".$k." Symb.S=".$ss;					        					        
					    }					   
					    my $nb_iter = get_nb_iter_for_params_with_n1($dbh,$k,$n1,$ss,$cr,$codec);
					    if ($nb_iter > $max_nb_iter)
					    {
						    $max_nb_iter = $nb_iter;
					    }
					    my $r = $k / Math::Fraction::frac($cr) - $k;
					    my $prep = $dbh->prepare("select count(d.decoding_status),i.iter_nb_received_symbols
					    from run_table r,decoding_table d, iter_table i, codec_table c
					 where d.decoding_id = i.decoding_id and d.decoding_status = 0 and 
					 r.run_k=? and 
					 r.run_id=i.run_id and 
					 c.codec_id = r.codec_id and 
					 r.codec_id = ? and 
					 r.run_left_degree = ? and
					 r.run_symbol_size = ?
					and r.run_r = ? 
					group by i.iter_nb_received_symbols");
					    $prep->execute($k,$codec,$n1,$ss,$r) or die "unable to find valid data";
					    while (@res = $prep->fetchrow_array())
					    {
						push @tab_decoding_success,$res[0]; 
						push @tab_nb_received_symbols,$res[1];
						push @tab_nb_samples,$res[0];
						print "$res[1]:$res[0]\n";
					    }
					    # we now calculate the number of failures for each point, i.e. each value of the number
					    # of received symbols where we have a non null number of decoding success.
					    # To that purpose, instead of calculating: nb_iter - sum of the number of successes so far
					    # we calculate from the end, in the reverse direction, as the number of successes after the
					    # the current situation. That's totally equivalent.
					    # NB: if initially tab_decoding_success contains the number of successes for a given number
					    # of received symbols, this is no longer the case at the end (it contains the number of
					    # failures)
					    for (my $i=0;$i<(scalar @tab_decoding_success -1);$i++)
					    {
						$total_nb_samples+=$tab_decoding_success[$i];
						$tab_decoding_success[$i] =$tab_decoding_success[$i+1];
					    }
					    $total_nb_samples+=$tab_decoding_success[(scalar @tab_decoding_success -1)];
					   	$tab_decoding_success[(scalar @tab_decoding_success -1)]=0;
					    for (my $i=(scalar @tab_decoding_success -2);$i>=0;$i--)
					    {
						$tab_decoding_success[$i] +=$tab_decoding_success[$i+1];
						#print "tab:".$tab_decoding_success[$i]."\n";
					    }
					    open(F_DAT,">$tmp_file_data") || die "Could not open $tmp_file_data\n";
					    print F_DAT "".($tab_nb_received_symbols[0])." ".($tab_decoding_success[0]/$max_nb_iter)." ".0;
					    print F_DAT "\n";
					    for (my $i=0;$i <scalar @tab_decoding_success-1;$i++)
					    {
							print F_DAT "".($tab_nb_received_symbols[$i])." ".($tab_decoding_success[$i]/$nb_iter)." ".$tab_nb_samples[$i];
							print F_DAT "\n";
							# plot intermediate points
							if ($i != scalar @tab_decoding_success-2)
							{
								# only if there is intermediate points
								my $delta = $tab_nb_received_symbols[$i+1]-$tab_nb_received_symbols[$i];
								if ($delta >0)
								{
									for (my $j=$tab_nb_received_symbols[$i]+1;$j<$tab_nb_received_symbols[$i+1];$j++)
									{
										print F_DAT "".($j)." ".($tab_decoding_success[$i+1]/$nb_iter)." ".$tab_nb_samples[$i+1];
										print F_DAT "\n";
									}
								}
							}							
					    }
					    close(F_DAT);
					    $prep->finish();
					    push @data_files,$tmp_file_data;
					    push @data_files_titles,$tmp_title;
					    push @nb_samples,$tab_decoding_success[0];
				}
			}
			else
			{
					    my @tab_nb_received_symbols;
					    my @tab_decoding_success;
					    my @tab_nb_samples;
					    my $tmp_file_data = $$params{"pref_1"}[0] . "test_nb_received_symbols_on_decoding_failure_codec_" .$_. "_k_". $k . "_ss_". $ss ."_code_rate_".$cr."_". $$params{"pref_2"}[0] . ".dat";
					    my $tmp_title = get_codec_name_for_codec_id($dbh,$codec). " CR=".Math::Fraction::frac($cr)." K=".$k." Symb.S=".$ss;
					    my $nb_iter = get_nb_iter_for_params_without_n1($dbh,$k,$ss,$cr,$codec);
					    if ($nb_iter > $max_nb_iter)
					    {
						    $max_nb_iter = $nb_iter;
					    }				
					    my $r = $k/Math::Fraction::frac($cr)-$k;	   
					    my $prep = $dbh->prepare("select count(d.decoding_status),avg(i.iter_nb_received_symbols) from run_table r,decoding_table d, iter_table i, codec_table c
					 where d.decoding_id = i.decoding_id and d.decoding_status = 0 and 
					 r.run_k=? and 
					 r.run_id=i.run_id and 
					 c.codec_id = r.codec_id and 
					 r.codec_id = ? and 
					 r.run_symbol_size = ?
					and r.run_r = ? 
					group by i.iter_nb_received_symbols");
					    $prep->execute($k,$codec,$ss,$r) or die "unable to find valid data";
					    while (@res = $prep->fetchrow_array())
					    {
						push @tab_decoding_success,$res[0]; 
						push @tab_nb_received_symbols,$res[1];
						push @tab_nb_samples,$res[0];
					    }
					    # we now calculate the number of failures for each point, i.e. each value of the number
					    # of received symbols where we have a non null number of decoding success.
					    # To that purpose, instead of calculating: nb_iter - sum of the number of successes so far
					    # we calculate from the end, in the reverse direction, as the number of successes after the
					    # the current situation. That's totally equivalent.
					    # NB: if initially tab_decoding_success contains the number of successes for a given number
					    # of received symbols, this is no longer the case at the end (it contains the number of
					    # failures)
					    for (my $i=0;$i<(scalar @tab_decoding_success -1);$i++)
					    {
						$total_nb_samples+=$tab_decoding_success[$i];
						$tab_decoding_success[$i] =$tab_decoding_success[$i+1];
					    }
					    $total_nb_samples+=$tab_decoding_success[(scalar @tab_decoding_success -1)];
					    for (my $i=(scalar @tab_decoding_success -2);$i>=0;$i--)
					    {
						$tab_decoding_success[$i] +=$tab_decoding_success[$i+1];
					    }				
					    open(F_DAT,">$tmp_file_data") || die "Could not open $tmp_file_data\n";
					    print F_DAT "".($tab_nb_received_symbols[0])." ".($tab_decoding_success[0]/$max_nb_iter)." ".0;
					    print F_DAT "\n";
					    for (my $i=0;$i <scalar @tab_decoding_success-1;$i++)
					    {
							print F_DAT "".($tab_nb_received_symbols[$i])." ".($tab_decoding_success[$i]/$nb_iter)." ".$tab_nb_samples[$i];
							print F_DAT "\n";
							# plot intermediate points
							if ($i != scalar @tab_decoding_success-2)
							{
								#only if there is intermediate points.
								my $delta = $tab_nb_received_symbols[$i+1]-$tab_nb_received_symbols[$i];
								if ($delta >0)
								{
									for (my $j=$tab_nb_received_symbols[$i]+1;$j<$tab_nb_received_symbols[$i+1];$j++)
									{
										print F_DAT "".($j)." ".($tab_decoding_success[$i+1]/$nb_iter)." ".$tab_nb_samples[$i+1];
										print F_DAT "\n";
									}
								}
							}							
					    }
					    close(F_DAT);
					    $prep->finish();
					    push @data_files,$tmp_file_data;
					    push @data_files_titles,$tmp_title;			
					    push @nb_samples,$tab_decoding_success[0];	    
			}
		}
	  }
    }
    
  open(F_DEM, ">$file_cmd") || die "Could not open $file_cmd\n";
  print F_DEM <<EOM;
set xlabel "nb_received symbols"
set ylabel "decoding failure probabilitiy"
set y2label "nb samples"
set autoscale
set nolabel
set nogrid
#set key box
set grid ytics mytics xtics
set ytics nomirror
set y2tics
set autoscale  y
set autoscale y2
set size 0.75,0.75
set logscale y
unset log y2
#set data style linespoints
EOM
  print F_DEM "set arrow from  $k,".1/$max_nb_iter." to $k,1  nohead lt -1\n";
  print F_DEM "set xrange [".($k-45).":".($k+50)."]\n";
  print F_DEM "set yrange [".1/$max_nb_iter.":1.1]\n";
  print F_DEM "set terminal postscript eps color\n";
  print F_DEM "set output \"".$file_output . "\"\n";
  print F_DEM "plot ";
  for (my $i=0;$i<scalar @data_files;$i++)
  {
    print F_DEM "\"$data_files[$i]\" using 1:2 title \" $data_files_titles[$i] \"  with linespoints lt ".($i+1);
    print F_DEM ",\"$data_files[$i]\" using 1:3 title \" nb samples (tot. :".$total_nb_samples." ) \"  with filledcurve lt ".($i+4)." axis x1y2 ";
    if ($i < (scalar @data_files-1)) { printf F_DEM ","; }
  }
  close(F_DEM); 
}

sub generate_inef_ratio_for_code_rate(dbh,params)
{
  my ($dbh,$params)=@_;
  my $file_cmd = $$params{"pref_1"}[0] . "test_inef_ration_for_code_rate_" . $$params{"pref_2"}[0] . ".dem";
  my $file_output = $$params{"pref_1"}[0] . "test_inef_ration_for_code_rate_" . $$params{"pref_2"}[0] . ".eps";
  my (@codec_ids,@codec_names);
  my @data_files;
  my @data_files_title;
  my $tmp_file = "./tmp_file_".`date +%Y-%m-%d_%Hh%Mm%S`;
    chop($tmp_file);
  
  get_codec_ids($dbh,\@codec_ids,\@codec_names);
  foreach my $k (get_all_k($dbh))
  {
	  foreach my $codec (@codec_ids)
	  {
	  	foreach my $ss (get_all_symbol_size($dbh))
	  	{
	  		if ($codec  >= 2)# ldpc codecs
	  		{
	  			my @N1;
	  			get_ldpc_N1($dbh,\@N1);
		  		foreach my $n1 (@N1)
		  		{
					    my @code_rates = get_list_of_code_rate_for_k($dbh,$k);
					    my @min;
					    my @moy;
					    my @max;
					    my @list_code_rates;
					    foreach my $cr (@code_rates)
					    {
					      my @res = get_min_max_conf_and_avg_inef_ratio_with_n1($dbh,"code_rate",$cr,$$params{"descr_stat"},$tmp_file,$n1,$codec,$ss,$k);					     
					      push (@min,@res[0]);
					      push (@moy,@res[1]);
					      push (@max,@res[2]);
					      push (@list_code_rates,$cr);
					    }
					      my $file_data;
					    my $tmp_title;
					    if ($codec==2) {
					    	$file_data = $$params{"pref_1"}[0] . "test_inef_ratio_for_code_rate_rs_m_". $n1 . "_k_" . $k . "_ss_". $ss . "_codec_" .$codec . $$params{"pref_2"}[0]  . ".dat";
					    	$tmp_title = get_codec_name_for_codec_id($dbh,$codec). " (m=".$n1.") K=".$k." Symb.S=".$ss;					    
					    }else{
					    	$file_data = $$params{"pref_1"}[0] . "test_inef_ratio_for_code_rate_left_degree_". $n1 . "_k_" . $k . "_ss_". $ss . "_codec_" .$codec . $$params{"pref_2"}[0]  . ".dat";
					    	$tmp_title = get_codec_name_for_codec_id($dbh,$codec). " (N1=".$n1.") K=".$k." Symb.S=".$ss;							    
					    }
					    my $i=0;

					    open(F_DAT,">$file_data") || die "Could not open $file_data\n";
					    while ($i < scalar @min)
					    {
					      print F_DAT "@list_code_rates[$i] @moy[$i] @min[$i] @max[$i]\n";
					      $i++;
					    }
					    close(F_DAT);
					    push @data_files,$file_data;
					    push @data_files_title,$tmp_title;
				}
			}
			else
			{
					    my @code_rates = get_list_of_code_rate_for_k($dbh,$k);
					    my @min;
					    my @moy;
					    my @max;
					    my @list_code_rates;
					    foreach my $cr (@code_rates)
					    {
					      my @res = get_min_max_conf_and_avg_inef_ratio_without_n1($dbh,"code_rate",$cr,$$params{"descr_stat"},$tmp_file,$codec,$ss,$k);					     
					      push (@min,@res[0]);
					      push (@moy,@res[1]);
					      push (@max,@res[2]);
					      push (@list_code_rates,$cr);
					    }
					    
					    my $file_data = $$params{"pref_1"}[0] . "test_inef_ratio_for_code_rate_k_" . $k . "_ss_". $ss . "_codec_" .$codec . $$params{"pref_2"}[0]  . ".dat";
					    my $tmp_title = get_codec_name_for_codec_id($dbh,$codec). " K=".$k." Symb.S=".$ss;
					    my $i=0;

					    open(F_DAT,">$file_data") || die "Could not open $file_data\n";
					    while ($i < scalar @min)
					    {
					      print F_DAT "@list_code_rates[$i] @moy[$i] @min[$i] @max[$i]\n";
					      $i++;
					    }
					    close(F_DAT);
					    push @data_files,$file_data;
					    push @data_files_title,$tmp_title;			
			}
		}
	}
  }

    open(F_DEM, ">$file_cmd") || die "Could not open $file_cmd\n";
    print F_DEM <<EOM;
set xlabel \" code rate \"
set ylabel "min 99 % conf/aver/max 99 % conf inef ratio"
set autoscale
set nolabel
set nogrid
#set key box
set grid ytics mytics xtics
#set xrange [30:50]
#set size 0.75,0.75
#set yrange [0:1.1]
#set logscale y
set data style linespoints
EOM
	  print F_DEM "set terminal postscript eps color\n";
	  print F_DEM "set output \"".$file_output . "\"\n";
		  #my $title = "test_k_impacts";
	  print F_DEM "plot ";
	  for (my $i=0;$i<scalar @data_files;$i++)
	  {

		print F_DEM "\"$data_files[$i]\" using 1:2 title \" @data_files_title[$i] \"  with lines lt ".($i+1)." ,";

		 print F_DEM "\"$data_files[$i]\" using 1:2:3:4 notitle with errorbars lt ".($i+1);
   		 if ($i < (scalar @data_files-1)) { printf F_DEM ","; }
	  }

    close(F_DEM);
}

sub generate_inef_ratio_for_object_size(dbh,params,code_rate,symbol_size,tmp_file)
{
  my ($dbh,$params)=@_;
  my $file_cmd = $$params{"pref_1"}[0] . "test_inef_ratio_for_k_" . $$params{"pref_2"}[0] . ".dem";
  my $file_output = $$params{"pref_1"}[0] . "test_inef_ratio_for_k_" . $$params{"pref_2"}[0] . ".eps";
  my (@codec_ids,@codec_names,@code_rates);
  get_codec_ids($dbh,\@codec_ids,\@codec_names);
    my @data_files;
    my @data_files_title;
  my $tmp_file = "./tmp_file_".`date +%Y-%m-%d_%Hh%Mm%S`;
    chop($tmp_file);
  foreach my $cr (get_all_readable_code_rate($dbh,\@code_rates))
  {
  	 foreach my $codec (@codec_ids)
  	 {
	  	foreach my $ss (get_all_symbol_size($dbh))
	  	{
	  		if ($codec  >= 2)# ldpc codecs
	  		{
	  			my @N1;
	  			get_ldpc_N1($dbh,\@N1);
		  		foreach my $n1 (@N1)
		  		{
				    my @k_list = get_list_of_k_for_ratio_with_n1($dbh,$cr,$codec,$ss,$n1);
				    my @min;
				    my @moy;
				    my @max;
				    my @list_k;
				    foreach my $k (@k_list)
				    {
				      my @res = get_min_max_conf_and_avg_inef_ratio_with_n1($dbh,"k",$k,$$params{"descr_stat"},$tmp_file,$n1,$codec,$ss,$cr);  
				      push (@min,@res[0]);
				      push (@moy,@res[1]);
				      push (@max,@res[2]);
				      push (@list_k,$k);
				    }
				    my $readable_code_rate = $cr;
  				    $readable_code_rate =~ s/\//_/;	
				    my $file_data;
				    my $tmp_title;
				    
				    if ($codec==2) {
				    	$file_data = $$params{"pref_1"}[0] . "inef_ratio_for_k_rs_m_" . $n1 . "_code_rate_" .$readable_code_rate . "_codec_" .$codec . "_ss_". $ss . $$params{"pref_2"}[0] .".dat";
				    	$tmp_title= get_codec_name_for_codec_id($dbh,$codec). " (m=".$n1.") CR=".$cr." Symb.S=".$ss;				    
				    }else{
				    	$file_data = $$params{"pref_1"}[0] . "inef_ratio_for_k_left_degree_" . $n1 . "_code_rate_" .$readable_code_rate . "_codec_" .$codec . "_ss_". $ss . $$params{"pref_2"}[0] .".dat";
				    	$tmp_title= get_codec_name_for_codec_id($dbh,$codec). " (N1=".$n1.") CR=".$cr." Symb.S=".$ss;				    
				    }
				    my $i=0;

				    open(F_DAT,">$file_data") || die "Could not open $file_data\n";
				    while ($i < scalar @min)
				    {
				      print F_DAT "@list_k[$i] @moy[$i] @min[$i] @max[$i]\n";
				      $i++;
				    }
				    close(F_DAT);
				    push @data_files,$file_data;
				    push @data_files_title,$tmp_title;
				}
			   }
			   else
			   {
				    my @k_list = get_list_of_k_for_ratio_without_n1($dbh,$cr,$codec,$ss);
				    my @min;
				    my @moy;
				    my @max;
				    my @list_k;
				    foreach my $k (@k_list)
				    {
				      my @res = get_min_max_conf_and_avg_inef_ratio_without_n1($dbh,"k",$k,$$params{"descr_stat"},$tmp_file,$codec,$ss,$cr);  
				      push (@min,@res[0]);
				      push (@moy,@res[1]);
				      push (@max,@res[2]);
				      push (@list_k,$k);
				    }
				    my $readable_code_rate = $cr;
  				    $readable_code_rate =~ s/\//_/;	
				    my $file_data = $$params{"pref_1"}[0] . "test_k_impacts_code_rate_" .$readable_code_rate. "_codec_" .$codec . "_ss_". $ss . $$params{"pref_2"}[0] .".dat";
				    my $tmp_title= get_codec_name_for_codec_id($dbh,$codec). " CR=".$cr." Symb.S=".$ss;

				    my $i=0;

				    open(F_DAT,">$file_data") || die "Could not open $file_data\n";
				    while ($i < scalar @min)
				    {
				      print F_DAT "@list_k[$i] @moy[$i] @min[$i] @max[$i]\n";
				      $i++;
				    }
				    close(F_DAT);
				    push @data_files,$file_data;
				    push @data_files_title,$tmp_title;			   
			   }
   		}
   	}
   }
   open(F_DEM, ">$file_cmd") || die "Could not open $file_cmd\n";
    print F_DEM <<EOM;
set xlabel \" object size (in symbols) \"
set ylabel "min 99 % conf/aver/max 99 % conf inef ratio"
set autoscale
set nolabel
set nogrid
#set key box
set grid ytics mytics xtics
#set xrange [30:50]
#set size 0.75,0.75
#set yrange [0:1.1]
#set logscale y
set data style linespoints
EOM
	  print F_DEM "set terminal postscript eps color\n";
	  print F_DEM "set output \"".$file_output . "\"\n";
		  #my $title = "test_k_impacts";
	  print F_DEM "plot ";
	  for (my $i=0;$i<scalar @data_files;$i++)
	  {

		print F_DEM "\"$data_files[$i]\" using 1:2 title \" @data_files_title[$i] \"  with lines lt ".($i+1)." ,";

		 print F_DEM "\"$data_files[$i]\" using 1:2:3:4 notitle with errorbars lt ".($i+1);
   		 if ($i < (scalar @data_files-1)) { printf F_DEM ","; }
	  }
    close(F_DEM);
}

# sub write_inef_ratio_graph(params,k,min,moy,max,xlabel,title)
# {
#   my ($params,$k,$min,$moy,$max,$x_label,$title) = @_;
#   my $file_cmd = $$params{"pref_1"}[0][0] . $title . $$params{"pref_2"}[0][0] . ".dem";
#   my $file_data = $$params{"pref_1"}[0][0] . $title . $$params{"pref_2"}[0][0] . ".dat";
#   my $file_output = $$params{"pref_1"}[0][0] . $title . $$params{"pref_2"}[0][0] . ".eps";
#   my $i=0;
# 
#   open(F_DAT,">$file_data") || die "Could not open $file_data\n";
#   while ($i < scalar @{$min})
#   {
#     print F_DAT "@{$k}[$i] @{$moy}[$i] @{$min}[$i] @{$max}[$i]\n";
#     $i++;
#   }
#   close(F_DAT);
# 
#   open(F_DEM, ">$file_cmd") || die "Could not open $file_cmd\n";
#   print F_DEM $x_label;
#   print F_DEM <<EOM;
# set ylabel "min 99 % conf/aver/max 99 % conf inef ratio"
# set autoscale
# set nolabel
# set nogrid
# #set key box
# set grid ytics mytics xtics
# #set xrange [30:50]
# #set size 0.75,0.75
# #set yrange [0:1.1]
# #set logscale y
# set data style linespoints
# EOM
# 	print F_DEM "set terminal postscript eps color\n";
# 	print F_DEM "set output \"".$file_output . "\"\n";
# 		#my $title = "test_k_impacts";
# 		print F_DEM "plot \"$file_data\" using 1:2:3:4 notitle  with errorbars lt 1 ,\"$file_data\" using 1:2 title \" $title\" with lines lt 1";
#   close(F_DEM);
# }

sub get_min_max_conf_and_avg_inef_ratio_with_n1()
{
  my ($dbh,$type,$var,$descr_stat,$tmp_file,$left_degree,$codec,$ss,$other) = @_;#if type=code_rate,other=k. if type=k,other=code_rate
  my @vals;
  my $res;
  my @mean;
  #min max 99% conf
  my @interv;
  my $interv_found = 0;
  my $mean_found = 0;
  my $tmp_file_stat = $tmp_file."_stats.txt";
  my $tmp_file_histo = $tmp_file."_histo*";
  my $prep;
  my $r;
  #recup data dans iter_table pour une variable donnée
  if ($type eq "k")
  {
	  $r = $var/Math::Fraction::frac($other) - $var; 
	$prep = $dbh->prepare("select d.decoding_inef from decoding_table d,iter_table i, run_table r where i.decoding_id = d.decoding_id and r.run_k = ? and r.run_id = i.run_id and r.run_left_degree= ? and r.codec_id=? and r.run_r= ?  and r.run_symbol_size=?");
      #$prep = $dbh->prepare("select d.decoding_inef from decoding_table d,iter_table i, run_table r where i.decoding_id = d.decoding_id and r.run_k = ? and r.run_id = i.run_id and r.run_left_degree= ? and r.codec_id=? and abs( $other - ((r.run_k)/(r.run_k+r.run_r))) < 0.001 and r.run_symbol_size=?");
      $prep->execute($var,$left_degree,$codec,$r,$ss) or die "unable to find valid data";
  }
  elsif ($type eq "code_rate")
  {
      $prep = $dbh->prepare("select d.decoding_inef from decoding_table d,iter_table i, run_table r where i.decoding_id = d.decoding_id and abs(r.run_k/(r.run_k+r.run_r)- $var) < 0.001 and r.run_id = i.run_id and r.codec_id=? and r.run_left_degree=? and r.run_symbol_size=? and r.run_k=?");
      $prep->execute($codec,$left_degree,$ss,$other) or die "unable to find valid data";
  }
  else
  {
      $prep = "";
  }

  my $min=999;
  my $max=-999;
  while ($res = $prep->fetchrow_array())
  {
    push (@vals,$res);
    	if ($res < $min)
	{
		$min=$res;
	}
	if ($res > $max)
	{
		$max=$res;
	}
  }
  $prep->finish();
 
  `rm $tmp_file 2>/dev/null`;
  open(TMP_FILE,">$tmp_file") || die "Could not open $tmp_file\n";
  
  foreach(@vals)
  {
      print TMP_FILE "$_\n";
  }
  close(TMP_FILE);
  `$descr_stat 1 $tmp_file noninter >/dev/null`;
  open(I_FILE, "<$tmp_file_stat") || die "Could not open $tmp_file_stat\n";
  while (<I_FILE>) {
      SEARCH: {
	/mean/ && do {
	    if ($mean_found == 0)
	    {
	      @mean = split(/\s*=\s*/, $_);
	      chop(@mean[1]);
	      $mean_found=1;
	    }
	};
	/99:/ && do {
	    if( $interv_found ==0)
	    {
	      @interv = split(/\s*\+\/\-\s*/, $_);
	      $interv_found = 1;
	    }
	};
      }
  }
  close(I_FILE);
   
   if ($min < (@mean[1] - @interv[1]))
   {
	$min = (@mean[1] - @interv[1])
   }
   if ($max > (@mean[1] + @interv[1]))
   {
	$max = (@mean[1] + @interv[1])
   }
   `rm $tmp_file 2>/dev/null`;
   `rm $tmp_file_stat 2>/dev/null`;  
   `rm $tmp_file_histo 2>/dev/null`; 
  
  return ($min,@mean[1],$max);
}

sub get_min_max_conf_and_avg_inef_ratio_without_n1()
{
  my ($dbh,$type,$var,$descr_stat,$tmp_file,$codec,$ss,$other) = @_; #if type=code_rate,other=k. if type=k,other=code_rate
  my @vals;
  my $res;
  my @mean;

  #min max 99% conf
  my @interv;
  my $interv_found = 0;
  my $mean_found = 0;
  my $tmp_file_stat = $tmp_file."_stats.txt";
  my $tmp_file_histo = $tmp_file."_histo*";
  my $prep;
  my $r;
  #recup data dans iter_table pour une variable donnée
  if ($type eq "k")
  {
	 $r = $var/Math::Fraction::frac($other) - $var;
	$prep = $dbh->prepare("select d.decoding_inef from decoding_table d,iter_table i, run_table r where i.decoding_id = d.decoding_id and r.run_k = ? and r.run_id = i.run_id and r.codec_id=? and r.run_symbol_size=? and r.run_r = ?");
      #$prep = $dbh->prepare("select d.decoding_inef from decoding_table d,iter_table i, run_table r where i.decoding_id = d.decoding_id and r.run_k = ? and r.run_id = i.run_id and r.codec_id=? and r.run_symbol_size=? and abs((0.0+r.run_k)/(r.run_k+r.run_r)- ?) < 0.001");
    $prep->execute($var,$codec,$ss,$r) or die "unable to find valid data";
  }
  elsif ($type eq "code_rate")
  {
	my $r = $other/Math::Fraction::frac($var)-$other;
      $prep = $dbh->prepare("select d.decoding_inef from decoding_table d,iter_table i, run_table r where i.decoding_id = d.decoding_id and r.run_r = ? and r.run_id = i.run_id and r.codec_id=? and r.run_symbol_size=? and r.run_k=?");
      $prep->execute($r,$codec,$ss,$other) or die "unable to find valid data";
  }
  else
  {
      $prep = "";
  }

  my $min=999;
  my $max=-999;
  while ($res = $prep->fetchrow_array())
  {
    push (@vals,$res);
    	if ($res < $min)
	{
		$min=$res;
	}
	if ($res > $max)
	{
		$max=$res;
	}
  }
  $prep->finish();
 
  `rm $tmp_file 2>/dev/null`;
  open(TMP_FILE,">$tmp_file") || die "Could not open $tmp_file\n";
  
  foreach(@vals)
  {
      print TMP_FILE "$_\n";
  }
  close(TMP_FILE);

  `$descr_stat 1 $tmp_file noninter >/dev/null`;
  open(I_FILE, "<$tmp_file_stat") || die "Could not open $tmp_file_stat\n";
  while (<I_FILE>) {
      SEARCH: {
	/mean/ && do {
	    if ($mean_found == 0)
	    {
	      @mean = split(/\s*=\s*/, $_);
	      chop(@mean[1]);
	      $mean_found=1;
	    }
	};
	/99:/ && do {
	    if( $interv_found ==0)
	    {
	      @interv = split(/\s*\+\/\-\s*/, $_);
	      $interv_found = 1;
	    }
	};
      }
  }
  close(I_FILE);
   
   if ($min < (@mean[1] - @interv[1]))
   {
	$min = (@mean[1] - @interv[1])
   }
   if ($max > (@mean[1] + @interv[1]))
   {
	$max = (@mean[1] + @interv[1])
   }
   `rm $tmp_file 2>/dev/null`;
   `rm $tmp_file_stat 2>/dev/null`;  
   `rm $tmp_file_histo 2>/dev/null`; 
  
  return ($min,@mean[1],$max);
}

sub get_list_of_k_for_ratio_with_n1(dbh,ratio,codec,ss,n1)
{
  my ($dbh,$ratio,$codec,$ss,$n1)=@_;
  my $res;
  my @list;
  print "ratio=$ratio,$codec,$ss,$n1.\n";
  my $prep = $dbh->prepare("select run_k from run_table where abs((run_k/(run_k+run_r))-$ratio)<0.001 and codec_id=? and run_symbol_size=? and run_left_degree=?");
  $prep->execute($codec,$ss,$n1) or die "unable to find valid data";
  while ($res = $prep->fetchrow_array())
  {
    push (@list,$res);
  }
  $prep->finish();
  return sort {$a <=> $b} @list;
}

sub get_list_of_k_for_ratio_without_n1(dbh,ratio,codec,ss)
{
  my ($dbh,$ratio,$codec,$ss)=@_;
  my $res;
  my @list;
  my $prep = $dbh->prepare("select run_k from run_table where abs((run_k/(run_k+run_r))-$ratio)<0.001 and codec_id=? and run_symbol_size=?");
  $prep->execute($codec,$ss) or die "unable to find valid data";
  while ($res = $prep->fetchrow_array())
  {
    push (@list,$res);
  }
  $prep->finish();
  return sort {$a <=> $b} @list;
}

sub get_list_of_k_for_code_rate()
{
	my ($dbh,$cr)=@_;
	  my $res;
	  my @list;
	  my $prep = $dbh->prepare("select run_k from run_table where abs((run_k/round(run_k+run_r,2))-".$cr.")<0.01 group by run_k");
	  $prep->execute() or die "unable to find valid data";
	  while ($res = $prep->fetchrow_array())
	  {
	    push (@list,$res);
	  }
	  $prep->finish();
	  return sort {$a <=> $b} @list;	
}

sub get_codec_name_for_codec_id()
{
	my ($dbh,$id) = @_;
	my $name;
	my @res;
	my $prep = $dbh->prepare("select codec_name from codec_table where codec_id='$id'");
	$prep->execute() or die "echec";
	while (@res = $prep->fetchrow_array())
	{
		$name = $res[0];
	}
	$prep->finish();	
	return $name;	
}

sub get_nb_iter_for_params_with_n1()
{
	my ($dbh,$k,$n1,$symbol_size,$code_rate,$codec) = @_;
	my $id;
	my $prep = $dbh->prepare("select count(i.iter_id) from decoding_table d,iter_table i, 
	run_table r where i.decoding_id = d.decoding_id and d.decoding_status = 0 and 
	i.run_id = r.run_id and r.run_k=? and  r.run_left_degree=? and 
	abs((0.0+r.run_k) / (r.run_k + r.run_r) - $code_rate) < 0.01 and r.run_symbol_size = ? and r.codec_id=?");
	$prep->execute($k,$n1,$symbol_size,$codec) or die "echec";
	$id =  $prep->fetchrow_array;
	$prep->finish(); 
	print "id=$id($k,$n1,$symbol_size,$code_rate,$codec)\n";
	return $id;	
}

sub get_nb_iter_for_params_without_n1()
{
	my ($dbh,$k,$symbol_size,$code_rate,$codec) = @_;
	my $id;
	my $r = $k / Math::Fraction::frac($code_rate) - $k;
	my $prep = $dbh->prepare("select count(i.iter_id) from decoding_table d,iter_table i, 
	run_table r where i.decoding_id = d.decoding_id and d.decoding_status = 0 and 
	i.run_id = r.run_id and r.run_k=?  and 
	r.run_r = ? and r.run_symbol_size = ? and r.codec_id=?");
	$prep->execute($k,$r,$symbol_size,$codec) or die "echec";
	$id =  $prep->fetchrow_array;
	$prep->finish(); 
	return $id;	
}

sub get_all_readable_code_rate
{
  my ($dbh,$code_rates) = @_;
  my $res;
  my @list;
  my $prep = $dbh->prepare("select round(run_k/round(run_r+run_k,2),2) from run_table group by round(run_k/round(run_r+run_k,2),2)");
  $prep->execute() or die "unable to find valid data";
  while ($res = $prep->fetchrow_array())
  {
    push (@{$code_rates},Math::Fraction::frac($res));
  }
  $prep->finish();
  sort {$a <=> $b} @{$code_rates};  
}

sub get_list_of_code_rate_for_k(dbh,k)
{
  my ($dbh,$k) = (shift,shift);
  my $res;
  my @list;
  my $prep = $dbh->prepare("select round(run_k/round(run_r+run_k,2),2) from run_table where run_k=? group by round(run_k/round(run_r+run_k,2),2)");
  $prep->execute($k) or die "unable to find valid data";
  while ($res = $prep->fetchrow_array())
  {
    push (@list,$res);
  }
  $prep->finish();
  return sort {$a <=> $b} @list;  
}

sub get_nb_iter(dbh,k,ss,code_rate)
{
  my ($dbh,$k,$symbol_size,$code_rate) = @_;
  my $id;
  my $r = $k / Math::Fraction::frac($code_rate) - $k;
  my $prep = $dbh->prepare("select count(i.iter_id) from decoding_table d,iter_table i, 
run_table r where i.decoding_id = d.decoding_id and d.decoding_status = 0 and 
i.run_id = r.run_id and r.run_k=? and  
r.run_r = ? and r.run_symbol_size = ?");
  $prep->execute($k,$r,$symbol_size) or die "echec";
  $id =  $prep->fetchrow_array;
  $prep->finish(); 
  return $id;
}

sub get_all_k()
{
	my $dbh=shift;
	my $res;
	my @list;
	my $prep = $dbh->prepare("select run_k from run_table group by run_k");
	$prep->execute() or die "unable to find valid data";
	while ($res = $prep->fetchrow_array())
	{
		push (@list,$res);
	}
	$prep->finish();
	return sort {$a <=> $b} @list;	
}

sub get_codec_ids(dbh)
{
  my ($dbh,$ids,$names) = @_;
  my @res;
  my $i=0;
  my @id_list; my @name_list;
  my $prep = $dbh->prepare("select r.codec_id,c.codec_name from run_table r, codec_table c where r.codec_id = c.codec_id group by r.codec_id");
  $prep->execute() or die "echec";
  while (@res = $prep->fetchrow_array())
  {
    $$ids[$i]=$res[0];$$names[$i]=$res[1];$i++;
  }
  $prep->finish();
}

sub get_all_symbol_size()
{
	my $dbh=shift;
	my $res;
	my @list;
	my $prep = $dbh->prepare("select run_symbol_size from run_table group by run_symbol_size");
	$prep->execute() or die "unable to find valid data";
	while ($res = $prep->fetchrow_array())
	{
	push (@list,$res);
	}
	$prep->finish();
	return sort {$a <=> $b} @list;	
}

sub get_ldpc_N1()
{
  my ($dbh,$N1) = @_;
  my @res;
  my $i=0;
  my @id_list; my @name_list;
  my $prep = $dbh->prepare("select run_left_degree from run_table group by run_left_degree;");
  $prep->execute() or die "echec";
  while (@res = $prep->fetchrow_array())
  {
    $$N1[$i]=$res[0];$i++;
  }
  $prep->finish();	
}

sub get_all_combination_for(dbh,var,comb)
{
  my ($dbh,$var,$comb)=@_;
  my @res;
  my $i=0;
  my $prep = $dbh->prepare("select ".$var." from run_table");
  $prep->execute() or die "error : trying to execute select ".$var." from run_table";
  while (@res = $prep->fetchrow_array())  
  {
    $$comb[$i++] = $res[0];
  }
  $prep->finish();
}

sub read_params_for_curve(param)
{
  my $param=shift;
  my $arg,my $val;
  foreach (@ARGV)
  {
    ($arg,$val) = split("=");
    SWITCH: for ($arg)
    {
     /curve/ && do { $$param{"curve"}=$val; last;};
     /nb_source_symbol/ && do { $$param{"curve_nb_source_symbol"}=$val; last;};
     /code_rate/ && do { $$param{"curve_code_rate"}=$val; last;};
     /help/ && do {help();};
    }
  }
}

#1 : decoding bitrate for loss percentage
#2 : decoding failure probability for loss percentage(code_rate)
#3 : decoding failure probability for number of received symbols(nb_source_symbol)
#4 : inefficiency ratio for code rate
#5 : inefficiency ratio for object size
#6 : number of XOR operations for  object size
#7 : number of XOR operations for loss percentage
sub help()
{
  print<<EOM;
Usage : 
  $0 <file_param> -curve=<number> [-code_rate=?|-nb_source_symbol=?]
Performance test for openfec library.
Params : 
  <file_param>		: the parameter file for running test
  -curve=<number>	: the number of curve you want to graph.
EOM
	print "\nThis script allows you to generate and graph with gnuplot a set of curves for the OpenFEC.org codes and codecs.\n\n";
	print "The first parameter to define is the parameter file which contains database configuration.\n";
	print "Then, you can specify the number of curve you want to graph :\n";
	print "\t -curve=1 : decoding bitrate for loss percentage\n";
	print "\t -curve=2 : decoding failure probability for loss percentage. You have to specify a code rate.\n";
	print "\t -curve=3 : decoding failure probability for number of received symbols. You have to specify a number of source symbol.\n";
	print "\t -curve=4 : inefficiency ratio for code rate.\n";
	print "\t -curve=5 : inefficiency ratio for object size.\n";
	print "\t -curve=6 : number of XOR operations for  object size.\n";
	print "\t -curve=7 : number of XOR operations for loss percentage.\n";
    print "\t -curve=8 : encoding bitrate for loss percentage\n";
	print "\nExemple : $0 params.txt -curve=2 code_rate=2/3\n";
	exit;
}

#read params in file
sub read_params(file,hash)
{
  my ($file,$hash)=(shift,shift);
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
  $$param{"pref_1"}[0] = "doom__";#`date +%Y-%m-%d_%Hh%M`;
  $$param{"pref_2"}[0] = "";
  $$param{"database_file"} = "";	
  $$param{"descr_stat"} = "./descr_stats";
}

