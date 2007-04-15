package Taxbird::UI::PinCode;
## 
## Copyright (C) 2007  Stefan Siegl <stesie@brokenpipe.de>, Germany
## 
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
## 
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
## 

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

