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

(tb:eval-file "storage.scm")
(tb:eval-file "basics-addresse.scm")
(tb:eval-file "basics-datenlieferant.scm")
(tb:eval-file "bundesland-chooser.scm")
(tb:eval-file "steuernummer.scm")

(define ustva-2005:definition
  '("Allgemeine Daten" '(tb:field:chooser
			 bundesland:chooser

			 tb:field:text-input
			 '("stnr" "Steuernummer"
			   (string-append "Die vom zuständigen Finanzamt "
					  "vergebene Steuernummer. "
					  "Eingabe mit Schrägstrichen.")
			   steuernummer:validate)

			 tb:field:chooser
			  '("zeitraum" "Voranmeldungszeitraum"
			    "Umsatzsteuervoranmeldungszeitraum"
			    ("Januar" "Februar" "März" "April" "Mai" "Juni"
			     "Juli" "August" "September" "Oktober" "November"
			     "Dezember" "1. Quartal" "2. Quartal" "3. Quartal"
			     "4. Quartal")))

    "Datenlieferant" basics:datenlieferant
    "Steuerpflichtiger" basics:adresse 

    "Umsatzsteuer" '(tb:field:text-input-calc
		     '("kz01" "Bemessungsgrundlage"
		       "blafasel Dokumentation 12345 ..."
		       tb:validate-numerical
		       "kz02"))
    ))


(tb:form-register
 ; form's name and definition
 "Umsatzsteuervoranmeldung 2005" ustva-2005:definition

 ; retrieval function
 storage:retrieve

 ; storage function
 storage:store

 ; empty set
 (lambda () '(("vend-id" . "74931"))))

