;; Copyright(C) 2004,05 Stefan Siegl <ssiegl@gmx.de>
;; taxbird - free program to interface with German IRO's Elster/Coala
;;
;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2 of the License, or
;; (at your option) any later version.
;;
;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with this program; if not, write to the Free Software
;; Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

(tb:eval-file "validate.scm")

(define basics:adresse
  (list 1 ;; one column only
	
	(list "Name"
	      tb:field:text-input
	      "name"
	      "Name des Steuerpflichtigen"
	      (lambda(value buf)
		(validate:alphanum value 1 45)))

	(list "Straße" 
	      tb:field:text-input
	      "strasse"
	      (string-append "Straßenname und Hausnummer "
			     "(der Anschrift des Steuerpflichtigen)")
	      (lambda(value buf)
		(validate:alphanum value 1 30)))

	(list "PLZ"
	      tb:field:text-input
	      "plz"
	      "Postleitzahl (der Anschrift des Steuerpflichtigen)"
	      (lambda(value buf)
		(validate:alphanum value 1 12)))

	(list "Ort"
	      tb:field:text-input
	      "ort"
	      "Wohnort (der Anschrift des Steuerpflichtigen)"
	      (lambda(value buf)
		(validate:alphanum value 1 30)))))
