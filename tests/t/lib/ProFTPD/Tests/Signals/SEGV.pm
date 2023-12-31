package ProFTPD::Tests::Signals::SEGV;

use lib qw(t/lib);
use base qw(ProFTPD::TestSuite::Child);
use strict;

use File::Spec;
use IO::Handle;

use ProFTPD::TestSuite::FTP;
use ProFTPD::TestSuite::Utils qw(:auth :config :running :test :testsuite);

$| = 1;

my $order = 0;

my $TESTS = {
  segv_daemon_ok => {
    order => ++$order,
    test_class => [qw(bug)],
  },
};

sub new {
  return shift()->SUPER::new(@_);
}

sub list_tests {
  return testsuite_get_runnable_tests($TESTS);
}

sub server_segfault {
  my $pid_file = shift;

  my $pid;
  if (open(my $fh, "< $pid_file")) {
    $pid = <$fh>;
    chomp($pid);
    close($fh);

  } else {
    croak("Can't read $pid_file: $!");
  }

  unless (kill('SEGV', $pid)) {
    print STDERR "Error sending SIGSEGV to PID $pid: $!\n";
  }
}

sub segv_daemon_ok {
  my $self = shift;
  my $tmpdir = $self->{tmpdir};

  my $config_file = "$tmpdir/signals.conf";
  my $pid_file = File::Spec->rel2abs("$tmpdir/signals.pid");
  my $scoreboard_file = File::Spec->rel2abs("$tmpdir/signals.scoreboard");

  my $log_file = File::Spec->rel2abs('tests.log');

  my $config = {
    PidFile => $pid_file,
    ScoreboardFile => $scoreboard_file,
    SystemLog => $log_file,

    TraceLog => $log_file,
    Trace => 'DEFAULT:10',

    IfModules => {
      'mod_delay.c' => {
        DelayEngine => 'off',
      },
    },
  };

  my ($port, $config_user, $config_group) = config_write($config_file, $config);

  my $ex;

  # Start server
  server_start($config_file);

  # Allow a short interval between startup and shutdown
  sleep(1);

  # Segfault the server
  server_segfault($pid_file);

  # Make sure that the pid file has been removed by the server as part of
  # its shutdown procedures.  We need the delay since proftpd handles
  # signals synchronously; it make take a short while for proftpd to
  # process the SIGSEGV and shut down all of the way.

  sleep(1);

  if (-e $pid_file) {
    die("Unclean shutdown: PidFile $pid_file still present");
  }

  if (-e $scoreboard_file) {
    die("Unclean shutdown: ScoreboardFile $scoreboard_file still present");
  }

  if ($ex) {
    die($ex);
  }

  unlink($log_file);
}

1;
