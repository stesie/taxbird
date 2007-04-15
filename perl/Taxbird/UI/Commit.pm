package Taxbird::UI::Commit;

use strict;
use warnings;

use Glib qw(TRUE FALSE);
use Gtk2 -init;
use Gtk2::GladeXML;
use Carp;
use Geier;
use Taxbird::UI::PinCode;

sub new { return bless({}, shift)->_init; }

sub run { 
  my $self = shift->new; 
  $self->{xmltree} = shift;

  # validate provided Elster XML-tree
  $self->{coala} = new Geier();
  croak "Elster-XML-Datensatz ungültig"
    if $self->{coala}->validate($self->{xmltree});

  Gtk2->main; 
}

sub _init {
  my $self = shift;

  # load interface
  my $glade_file = __FILE__;
  $glade_file =~ s/Commit.pm/Commit.glade/;
  $self->{gui} = Gtk2::GladeXML->new($glade_file, 'commit-auth-dialog')
    or die "unable to create application window";

  $self->fill_cert_list;
  $self->{gui}->signal_autoconnect_from_package($self);
  $self->{window} = $self->{gui}->get_widget('commit-auth-dialog') or die;

  return $self;
}

sub fill_cert_list {
  my $self = shift;

  # little hack: in order to force the combo box to be a text selection
  # box we added one dummy entry in glade, therefore we have to remove 
  # this one first
  my $combo = $self->{gui}->get_widget('auth_opensc_cert') or die;
  $combo->remove_text(0);

  $self->{certs} = eval {
    use Taxbird::OpenSC;
    Taxbird::OpenSC::get_opensc_certificates();
  };

  if(ref($self->{certs}) ne "HASH") {
    # no certificates available, disable selection part of dialog
    foreach('auth_opensc_cert', 'auth_opensc_label', 'auth_opensc') {
      my $widget = $self->{gui}->get_widget($_) or die;
      $widget->set_sensitive(FALSE);
    }
    return;
  }

  foreach (keys %{$self->{certs}}) {
    $combo->append_text($_);
  }

  $combo->set_active(0);
}

sub on_commit_auth_cancel {
  my $self = shift;
  $self->{window}->destroy;
  Gtk2->main_quit;
}

sub on_commit_auth_delete {
  my $self = shift;
  Gtk2->main_quit;
  return FALSE;
}

sub on_commit_auth_ok {
  my $self = shift;

  my $auth_none = $self->{gui}->get_widget('auth_none') or die;
  my $auth_softpse = $self->{gui}->get_widget('auth_softpse') or die;
  my $auth_opensc = $self->{gui}->get_widget('auth_opensc') or die;

  # digitally sign the document, if needed
  if($auth_none->get_active) {
    # nothing to do ...
  }
  elsif($auth_softpse->get_active) {
    my $softpse_fn = $self->{gui}->get_widget('auth_softpse_filename') or die;
    my $fn = $softpse_fn->get_filename;

    # check whether file exists
    unless(defined $fn && -e $fn) {
      $self->error_dialog("Die angegebene Zertifikatscontainerdatei existiert nicht.");
      return;
    }

    my $pincode = Taxbird::UI::PinCode->run or return;

    # sign using softpse
    my $result = $self->{coala}->sign_softpse($self->{xmltree}, $fn, $pincode);
    unless($result) {
      $self->error_dialog("Der XML-Datensatz konnte nicht signiert werden.  PIN-Code falsch?");
      return;
    }

    $self->{xmltree} = $result;
  }
  elsif($auth_opensc->get_active) {
    # figure out cert id
    my $opensc_cert = $self->{gui}->get_widget('auth_opensc_cert') or die;
    my $certname = $opensc_cert->get_active_text;
    my $certid = $self->{certs}->{$certname};

    # do it
    my $result = $self->{coala}->sign_opensc($self->{xmltree}, $certid);
    unless($result) {
      $self->error_dialog("Der XML-Datensatz konnte nicht signiert werden.  PIN-Code falsch?");
      return;
    }

    $self->{xmltree} = $result;
  }
  else {
    die;
  }
}

sub error_dialog {
  my $self = shift;
  my $msg = shift;

  my $dialog = Gtk2::MessageDialog->new
    ($self->{window}, 'destroy-with-parent', 'error', 'close', $msg);

  $dialog->run;
  $dialog->destroy;
}

1;

