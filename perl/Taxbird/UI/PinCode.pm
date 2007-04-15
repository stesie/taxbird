package Taxbird::UI::PinCode;

use strict;
use warnings;

use Glib qw(TRUE FALSE);
use Gtk2 -init;
use Gtk2::GladeXML;
use Carp;

sub new { return bless({}, shift)->_init; }

sub run { 
  my $self = shift->new; 
  my $response = $self->{dialog}->run;

  # handle reply
  if($response eq 'cancel') {
    $response = undef;
  }
  else {
    my $textbox = $self->{handle}->get_widget('pincode') or die;
    $response = $textbox->get_text;
  }

  $self->{dialog}->destroy;
  return $response;
}

sub _init {
  my $self = shift;

  # load interface
  my $glade_file = __FILE__;
  $glade_file =~ s/PinCode.pm/PinCode.glade/;
  
  $self->{handle} = Gtk2::GladeXML->new($glade_file, 'pincode-dialog')
    or die "unable to create application window";
  $self->{dialog} = $self->{handle}->get_widget('pincode-dialog') or die;

  return $self;
}

1;

