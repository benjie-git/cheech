#!/usr/bin/perl -w

use Socket;
use Net::DNS;

my $failed = 0;
my $verbose = 0;


my $bin_dir = "../examples/";
my $dnslookup = $bin_dir . "dnslookup";



sub main()
{
    test_args("");             # Blocking, list
    test_args("-a");           # Asynchronous, list
    test_args("-s");           # Blocking, single
    test_args("-a -s");        # Asynchronous, single
    test_args("-n 25");        # Blocking, list, x25
    test_args("-n 25 -a");     # Asynchronous, list, x25
    test_args("-n 25 -s");     # Blocking, single, x25
    test_args("-n 25 -a -s");  # Blocking, list, x25

    rtest_args("-r");          # Blocking, reverse
    rtest_args("-r -a");       # Asynchronous, reverse
    rtest_args("-r -n 25");    # Blocking, reverse, x25
    rtest_args("-r -a -n 25"); # Asynchronous, reverse, x25

    exit 1 if ($failed);
    exit 0;
}


sub test_args
{
    my ($args) = @_;
    test("141.213.11.124",      "A",    $args);
    test("141.213.11.124",      "AAAA", $args);
    test("DEAD::BEEF", 		"A",    $args);
    test("DEAD::BEEF", 		"AAAA", $args);
    test("www.gnetlibrary.org", "A",    $args);
    test("www.yahoo.com",       "A",    $args);
    test("www.kame.net",        "A",    $args);
    test("www.gnetlibrary.org", "AAAA", $args);
    test("www.kame.net",        "AAAA", $args);
}


sub rtest_args
{
    my ($args) = @_;
    test("141.213.11.124",      "PTR",  $args);
    test("64.58.76.223",        "PTR",  $args);
    test("3FFE:501:4819:2000:210:F3FF:FE03:4D0", "PTR", $args);
}



sub test
{
    my ($hostname, $type, $args) = @_;
    my $name = $hostname . " " . $type . " (" . $args . ")";
    my $gnet_res = gnet_dns_lookup($hostname, $type, $args);
    my $good_res = good_dns_lookup($hostname, $type);
    if ($args !~ /-r/)
    {
	check ($name, $good_res, $gnet_res, $args);
    }
    else
    {
	rcheck ($name, $good_res, $gnet_res, $args);
    }
}


sub check
{
    my ($name, $good_res, $gnet_res, $args) = @_;

    printf("%-50s", $name . ":");

    if (defined($good_res) && defined($gnet_res))
    {
	@sgood_res = sort(@$good_res);
	@sgnet_res = sort(@$gnet_res);

	my $pass = 0;
	if ($args =~ /-s/)
	{
	    my $gnet_res = pop(@$gnet_res);
	    foreach my $goodr (@sgood_res)
	    {
		$pass = 1 if $goodr = $gnet_res;
	    }
	}
	elsif (@sgood_res == @sgnet_res)
	{
	    $pass = 1;
	}

	if ($pass)
	{
	    print "PASS\n";
	    if ($verbose)
	    {
		print "  Good:\n   ", join("\n   ", @sgood_res), "\n";
		print "  GNet:\n   ", join("\n   ", @sgnet_res), "\n";
	    }
	}
	else
	{
	    print "FAIL (result mismatch)\n";
	    print "  Good:\n   ", join("\n   ", @sgood_res), "\n";
	    print "  GNet:\n   ", join("\n   ", @sgnet_res), "\n";
	    $failed = 1;
	}
    }
    elsif (!defined($good_res) && !defined($gnet_res))
    {
	print "PASS (both lookups failed)\n";
    }
    elsif (defined($good_res) && !defined($gnet_res))
    {
	print "FAIL (GNet lookup failed)\n";
	print "  Good:\n   ", join("\n   ", @$good_res), "\n";
    }
    elsif (!defined($good_res) && defined($gnet_res))
    {
	print "FAIL? (good lookup failed)\n";
	print "  GNet:\n   ", join("\n   ", @$gnet_res), "\n";
    }
}


sub rcheck
{
    my ($name, $good_res, $gnet_res, $args) = @_;

    printf("%-50s", $name . ":");

    if (defined($good_res) && defined($gnet_res))
    {
	if ($good_res eq $gnet_res)
	{
	    print "PASS\n";
	    if ($verbose)
	    {
		print "  Good: $good_res\n";
		print "  GNet: $gnet_res\n";
	    }
	}
	else
	{
	    print "FAIL (result mismatch)\n";
	    print "  Good: $good_res\n";
	    print "  GNet: $gnet_res\n";
	    $failed = 1;
	}
    }
    elsif (!defined($good_res) && !defined($gnet_res))
    {
	print "PASS (both lookups failed)\n";
    }
    elsif (defined($good_res) && !defined($gnet_res))
    {
	print "FAIL (GNet lookup failed)\n";
	print "  Good: $good_res\n";
    }
    elsif (!defined($good_res) && defined($gnet_res))
    {
	print "PASS? (good lookup failed)\n";
	print "  GNet: $gnet_res\n" if $verbose;
    }
}






sub good_dns_lookup
{
    my $hostname = shift;
    my $type = shift;
    my @res = ();

    if ($type eq "PTR")
    {
	my $res = Net::DNS::Resolver->new;
	my $query = $res->search($hostname, "PTR");
	if ($query) 
	{
	    foreach my $rr ($query->answer) 
	    {
		next unless $rr->type eq "PTR";
		return $rr->ptrdname;
	    }
	}

	return undef;
    }


    # GNet will resolve 141.213.11.124 in IPv6 mode.  So will we.
    if ($hostname =~ /\d+\.\d+.\d+.\d+/)
    {
	push (@res, $hostname);
	return \@res;
    }
    elsif ($hostname =~ /:/)
    {
	push (@res, $hostname);
	return \@res;
    }

    my $resolver = Net::DNS::Resolver->new;
    my $query = $resolver->search($hostname, $type);
    if ($query) 
    {
	foreach my $rr ($query->answer) 
	{
	    next unless $rr->type eq $type;
	    push(@res, $rr->address);
	}
	return undef if ($#res == -1);
	return \@res;
    }
    
    return undef;
}


sub gnet_dns_lookup
{
    my ($hostname, $type, $args) = @_;
    my @res = ();

    $args = "" if !defined($args);

    # Set type
    if ($type eq 'A')
    {
	$ENV{'GNET_IPV6_POLICY'} = "4";
    }
    elsif ($type eq 'AAAA')
    {
	$ENV{'GNET_IPV6_POLICY'} = "6";
    }

    # Set num (for using -n option)
    $num = 0;
    $num = 23 if ($args =~ /-n/);

    my @lines = `$dnslookup $args $hostname`;
    return undef if $?;

    foreach my $line (@lines)
    {
	if ($line =~ /^$num: \S+ -> (\S+)/)
	{
	    push(@res, $1);
	}
	elsif ($line =~ /DNS lookup failed/)
	{
	    return undef;
	}
    }

    return $res[0] if ($type eq "PTR" && $#res > -1);

    return \@res;
}



main();
