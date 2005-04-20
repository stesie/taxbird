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
(tb:eval-file "validate.scm")
(tb:eval-file "zeitraum-chooser.scm")



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Umsatzsteuervoranmeldung 2005                                           ;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define ustva-2005:definition
  (list



   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   "Allgemeine Daten"

   (list 1

	 bundesland:chooser

	 (list "Steuernummer"
	       tb:field:text-input
	       "stnr"
	       (string-append "Die vom zuständigen Finanzamt vergebene "
			      "Steuernummer. Eingabe mit Schrägstrichen.")
	       steuernummer:validate)

	 zeitraum:chooser)
   


   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   "Datenlieferant"    basics:datenlieferant



   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   "Steuerpflichtiger" basics:adresse 



    ;; Vorderseite der Umsatzsteuervoranmeldung ...
   "Lieferungen und sonstige Leistungen" 

   (list



    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    "Steuerfreie Umsätze"
    (list 2
	  (list "<big><b>Mit Vorsteuerabzug ...</b></big>")

	  (list "<b>Innergemeinschaftliche Lieferungen ...</b>")

	  (list "Abnehmer mit USt-ID"
		tb:field:text-input
		"Kz41"
		(string-append "Innergem. Lieferungen (§ 4 Nr. 1 Buchst. b "
			       "UStG) an Abnehmer mit USt-IdNr.")
		validate:signed-int)


	  (list "Abnehmer ohne USt-ID\n(neue Fahrzeuge)"
		tb:field:text-input
		"Kz44"
		(string-append "Innergem. Lieferungen neuer Fahrzeuge an "
			       "Abnehmer ohne Ust-IdNr.")
		validate:signed-int)


	  (list "Außerhalb eines Unternehmens\n(neue Fahrzeuge)"
		tb:field:text-input
		"Kz49"
		(string-append "Innergemeinschaftliche Lieferungen neuer "
			       "Fahrzeuge außerhalb eines Unternehmens "
			       "§ 2a UStG")
		validate:signed-int)

	  
	   ;;;;;;;;;;;;;;;;;;
	  (list "<b>Weitere steuerfreie Umsätze ...</b>")


	  (list "mit Vorsteuerabzug"
		tb:field:text-input
		"Kz43"
		(string-append "Weitere steuerfreie Umsätze mit Vorsteuer "
			       "abzug (z.B. Ausfuhrlieferungen, Umsätze nach "
			       "§ 4 Nr. 2 bis 7 UStG)")
		validate:signed-int)

	   ;;;;;;;;;;;;;;;
	  (list "<big><b>ohne Vorsteuerabzug ...</b></big>")
	  
	  (list "Umsätze nach § 4 Nr. 8 bis 28 UStG"
		tb:field:text-input
		"Kz48"
		(string-append "Umsätze nach § 4 Nr. 8 bis 28 UStG "
			       "(steuerfreie Umsätze ohne Vorsteuerabzug)")
		validate:signed-int))



    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    "Steuerpflichtige Umsätze"
    (list 2

	  (list "zum Steuersatz von 16%"
		tb:field:text-input
		"Kz51"
		(string-append "Lieferungen und sonstige Leistungen einschl. "
			       "unentgeltlicher Wertabgaben "
			       "zum Steuersatz von 16 v.H.")
		validate:signed-int

		;; second field ...
		tb:field:text-output
		"Kz51-calc" #f #t)

	  
	  
	  (list "zum Steuersatz von 7%"
		tb:field:text-input
		"Kz86"
		(string-append "Lieferungen und sonstige Leistungen einschl. "
			       "unentgeltlicher Wertabgaben "
			       "zum Steuersatz von 7 v.H.")
		validate:signed-int

		;; second field ...
		tb:field:text-output
		"Kz86-calc" #f #t)


	  (list "andere Steuersätze"
		tb:field:text-input
		"Kz35"
		(string-append "Lieferungen und sonstige Leistungen einschl. "
			       "unentgeltlicher Wertabgaben, Umsätze, die "
			       "anderen Steuersätzen unterliegen (Umsatz)")
		validate:signed-int

		;; second field
		tb:field:text-input
		"Kz36"
		"Umsätze, die anderen Steuersätzen unterliegen (Steuer)"
		(lambda(val buf)
		  (validate:signed-monetary-max val buf
						(storage:retrieve buf
								  "Kz35")))))



    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    "Umsätze nach § 24 UStG"
    (list 2

	  (list "Lief. übriges Gemeinschaftsgeb.,\nAbn. mit USt-IdNr."
		tb:field:text-input
		"Kz77"
		(string-append "Umsätze land- und forstwirtschaftl. Betriebe "
			       "nach § 24 UStG; Lieferungen in das übrige "
			       "Gemeinschaftsgebiet an Abnehmer *mit* "
			       "USt-IdNr.")
		validate:signed-int)


	  
	  (list "Umsätze, Steuer nach § 24 UStG"
		tb:field:text-input
		"Kz76"
		(string-append "Umsätze land- und forstwirtschaftl. Betriebe "
			       "nach § 24 UStG; Umsätze für die eine Steuer "
			       "nach § 24 UStG zu entrichten ist (Sägewerks"
			       "erzeugnisse, Getränke und alkohol. "
			       "Flüssigkeiten, z. B. Wein) - Umsatz")
		validate:signed-int

		;; second field ...
		tb:field:text-input
		"Kz80"
		(string-append "Umsätze land- und forstwirtschaftl. Betriebe "
			       "nach § 24 UStG; Umsätze für die eine Steuer "
			       "nach § 24 UStG zu entrichten ist (Sägewerks"
			       "erzeugnisse, Getränke und alkohol. "
			       "Flüssigkeiten, z. B. Wein) - Steuer")
		(lambda (val b)
		  (validate:signed-monetary-max val b
						(storage:retrieve b "Kz76"))))))



   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   "Innergemeinschaftliche Erwerbe"
   (list 2

	 (list "steuerfrei, § 4b UStG"
	       tb:field:text-input
	       "Kz91" 
	       "Steuerfreie innergemeinschaftliche Erwerbe nach § 4b UStG" 
	       validate:signed-int)
	 
	 
	 (list "zum Steuersatz von 16%"
	       tb:field:text-input
	       "Kz97"
	       (string-append "Steuerpflichtige innergemeinschaftliche Erwerbe "
			      "zum Steuersatz von 16 v.H.")
	       validate:signed-int
	       
	       ;; second field ...
	       tb:field:text-output
	       "Kz97-calc" #f #t)

	  
	 (list "zum Steuersatz von 7%"
	       tb:field:text-input
	       "Kz93"
	       (string-append "Steuerpflichtige innergemeinschaftliche Erwerbe "
			      "zum Steuersatz von 7 v.H.")
	       validate:signed-int
	       
	       ;; second field ...
	       tb:field:text-output
	       "Kz93-calc" #f #t)
	 
	 
	 (list "andere Steuersätze"
	       tb:field:text-input
	       "Kz95"
	       (string-append "Steuerpflichtige innergemeinschaftliche Erwerbe,"
			      " die anderen Steuersätzen unterliegen (Umsatz)")
	       validate:signed-int

	       ;; second field
	       tb:field:text-input
	       "Kz98"
	       (string-append "Steuerpflichtige innergemeinschaftliche Erwerbe,"
			      " die anderen Steuersätzen unterliegen (Steuer)")
	       (lambda(val buf)
		 (validate:signed-monetary-max val buf
					       (storage:retrieve buf "Kz95"))))
	 

	 (list "neue Fahrz., ohne USt-IdNr."
	       tb:field:text-input
	       "Kz94"
	       (string-append "Steuerpflichtige innergemeinschaftliche Erwerbe,"
			      " neuer Fahrzeuge von Lieferern *ohne* USt-IdNr. "
			      "zum allgemeinen Steuersatz (Umsatz)")
	       validate:signed-int
	       
	       ;; second field
	       tb:field:text-input
	       "Kz96"
	       (string-append "Steuerpflichtige innergemeinschaftliche Erwerbe"
			      ", neuer Fahrzeuge von Lieferern *ohne* "
			      "USt-IdNr. zum allgemeinen Steuersatz (Steuer)")
	       (lambda(val buf)
		 (validate:signed-monetary-max val buf
					       (storage:retrieve buf
								 "Kz94")))))


   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   "Ergänzende Angaben zu Umsätzen"
   (list 2

	 (list "Lief. d. 1. Abn. bei innerg. Dreiecksgesch."
	       tb:field:text-input
	       "Kz42"
	       (string-append "Umsätze aus Lieferungen des ersten Abnehmers "
			      "bei *innergemeinschaftlichen Dreiecksgeschäften*"
			      " (§ 25b Abs. 2 UStG)")
	       validate:signed-int)


	 (list "Steuerpfl. Umsätze § 13b UStG"
	       tb:field:text-input
	       "Kz60"
	       (string-append "Steuerpflichtige Umsätze im Sinne des § 13b "
			      "Abs. 1 Satz 1 Nr. 1 bis 5 UStG, für die der "
			      "*Leistungsempfänger* die *Steuer schuldet*")
	       validate:signed-int)

	 
	 (list "im Inland nicht steuerbare Umsätze"
	       tb:field:text-input
	       "Kz45"
	       "im Inland nicht steuerbare Umsätze"
	       validate:signed-int))



   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   "Umsätze § 13b Abs. 2 UStG"
   (list 2

	 (list "Leistg. v. Untern. im Ausland"
	       tb:field:text-input
	       "Kz52"
	       (string-append "Leistungen eines im Ausland ansässigen "
			      "Unternehmers (§ 13b Abs. 1 Satz 1 Nr. 1 und 5 "
			      "UStG), für die als Leistungsempfänger "
			      "Steuer geschuldet wird - Umsatz")
	       validate:signed-int

	       ;; 2nd field
	       tb:field:text-input
	       "Kz53"
	       (string-append "Leistungen eines im Ausland ansässigen "
			      "Unternehmers (§ 13b Abs. 1 Satz 1 Nr. 1 und 5 "
			      "UStG), für die als Leistungsempfänger "
			      "Steuer geschuldet wird - Steuer")
	       (lambda (val b)
		 (validate:signed-monetary-max val b
					       (storage:retrieve b "Kz52"))))


	 (list "Lief. sichergsübereig. Gegenst., Ums. GrEStG"
	       tb:field:text-input
	       "Kz73"
	       (string-append "Lieferungen sicherungsübereigneter Gegenstände "
			      "und Umsätze, die unter das GrEStG fallen "
			      "(§ 13b Abs. 1 Satz 1 Nr. 2 und 3 UStG), für die "
			      "als Leistungsempfänger Steuer geschuldet "
			      "wird - (Umsatz)")
	       validate:signed-int

	       ;; 2nd field
	       tb:field:text-input
	       "Kz74"
	       (string-append "Lieferungen sicherungsübereigneter Gegenstände "
			      "und Umsätze, die unter das GrEStG fallen "
			      "(§ 13b Abs. 1 Satz 1 Nr. 2 und 3 UStG), für die "
			      "als Leistungsempfänger Steuer geschuldet "
			      "wird - (Steuer)")
	       (lambda (val b)
		 (validate:signed-monetary-max val b
					       (storage:retrieve b "Kz73"))))


	 (list "Bauleistungen eines Untern. im Inland"
	       tb:field:text-input
	       "Kz84"
	       (string-append "Bauleistungen eines im Inland ansässigen "
			      "Unternehmers (§ 13b Abs. 1 Satz 1 Nr. 4 UStG), "
			      "für die als Leistungsempfänger Steuer geschuldet"
			      " wird - (Umsatz)")
	       validate:signed-int

	       ;; 2nd field
	       tb:field:text-input
	       "Kz85"
	       (string-append "Bauleistungen eines im Inland ansässigen "
			      "Unternehmers (§ 13b Abs. 1 Satz 1 Nr. 4 UStG), "
			      "für die als Leistungsempfänger Steuer geschuldet"
			      " wird - (Steuer)")
	       (lambda (val b)
		 (validate:signed-monetary-max val b
					       (storage:retrieve b "Kz84")))))

   "Abziehbare Vorsteuerbeträge"
   (list 2

	 (list "aus Rechnungen anderer Untern."
	       tb:field:text-input
	       "Kz66"
	       (string-append "Vorsteuerbeträge aus Rechnungen von anderen "
			      "Unternehmern (§ 15 Abs. 1 Satz 1 Nr. 1 UStG), "
			      "aus Leistungen im Sinne des § 13a Abs. 1 Nr. 6 "
			      "UStG (§ 15 Abs. 1 Satz 1 Nr. 5 UStG) und aus "
			      "innergemeinschaftlichen Dreiecksgeschäften "
			      "(§ 25b Abs. 5 UStG)")
	       validate:signed-monetary)


	 (list "Vorst. aus innergem. Erwerb"
	       tb:field:text-input
	       "Kz61"
	       (string-append "Vorsteuerbeträge aus dem innergemeinschaftlichen"
			      " Erwerb von Gegenständen (§ 15 Abs. 1 Satz 1 "
			      "Nr. 3 UStG)")
	       validate:signed-monetary)


	 (list "Einfuhrumsatzsteuer"
	       tb:field:text-input
	       "Kz62"
	       (string-append "Entrichtete Einfuhrumsatzsteuer "
			      "(§ 15 Abs. 1 Satz 1 Nr. 2 UStG)")
	       validate:signed-monetary)


	 (list "Vorsteuer § 13b"
	       tb:field:text-input
	       "Kz67"
	       (string-append "Vorsteuerbeträge aus Leistungen im Sinne "
			      "des § 13b Abs. 1 UStG "
			      "(§ 15 Abs. 1 Satz 1 Nr. 4 UStG)")
	       validate:signed-monetary)


	 (list "nach allg. Durchschnittsätze"
	       tb:field:text-input
	       "Kz63"
	       (string-append "Vorsteuerbeträge, die nach allgemeinen "
			      "Durchschnittsätzen berechnet sind "
			      "(§§ 23 und 23a UStG)")
	       validate:signed-monetary)


	 (list "Berichtigung des Vorsteuerabzugs"
	       tb:field:text-input
	       "Kz64"
      	       "Berichtigung des Vorsteuerabzugs (§ 15a UStG)"
	       validate:signed-monetary)

	 
	 (list "innerg. Lief. neuer Fahrz."
	       tb:field:text-input
	       "Kz59"
	       (string-append "Vorsteuerabzug für innergemeinschaftliche "
			      "Lieferungen neuer Fahrzeuge außerhalb eines "
			      "Unternehmens (§ 2a UStG) sowie von Klein"
			      "unternehmern im Sinne des § 19 Abs. 1 UStG "
			      "(§ 15 Abs. 4a UStG)")
	       validate:signed-monetary))

   "Sonstige"
   (list 2

	 (list "Wechsel Bestuerungsform sowie Nachsteuer"
	       tb:field:text-input
	       "Kz65"
	       (string-append "Steuer infolge Wechsels der Besteuerungsform "
			      "sowie Nachsteuer auf versteuerte Anzahlungen "
			      "wegen Steuersatzerhöhung")
	       validate:signed-monetary)


	 (list "Sonstiges"
	       tb:field:text-input
	       "Kz69"
	       (string-append "Steuerbeträge, die vom letzten Abnehmer eines "
			      "innergemeinschaftlichen Dreiecksgeschäfts "
			      "geschuldet werden (§ 25b Abs. 2 UStG), "
			      "in Rechnungen unrichtig oder unberechtigt "
			      "ausgewiesene Steuerbeträge (§ 14c UStG), "
			      "Steuerbeträge für Leistungen im Sinne des "
			      "§ 13a Abs. 1 Nr. 6 UStG sowie Steuerbeträge, "
			      "die nach § 6a Abs. 4 Satz 2 oder § 17 Abs. 1 "
			      "Satz 2 UStG geschuldet werden.")
	       validate:signed-monetary)


	 (list "Anrechnung Sonder-VZ"
	       tb:field:text-input
	       "Kz39"
	       (string-append "Anrechnung (Abzug) der festgesetzten "
			      "Sondervorauszahlung für Dauerfristverlängerung "
			      "(nur auszufüllen in der letzten Voranmeldung "
			      "des Besteuerungszeitraums, in der Regel "
			      "Dezember)")
	       validate:unsigned-int))))




(define ustva-2005:recalculate
  (lambda (buffer element value)
    (let ((list '("Kz51" (lambda(v buffer)
			   (storage:store buffer "Kz51-calc"
					  (/ (* (string->number v) 16) 100)))
		  "Kz86" (lambda(v buffer)
			   (storage:store buffer "Kz86-calc"
					  (/ (* (string->number v) 7) 100)))
		  "Kz93" (lambda(v buffer)
			   (storage:store buffer "Kz93-calc"
					  (/ (* (string->number v) 7) 100)))
		  "Kz97" (lambda(v buffer)
			   (storage:store buffer "Kz97-calc"
					  (/ (* (string->number v) 16) 100))))))

      (while (> (length list) 0)
	     (if (string=? (car list) element)
		 (let ()
		   (if (= (string-length value) 0)
		       (set! value "0"))
		   ((eval (cadr list) (current-module)) value buffer)))
	     (set! list (cddr list))))))




(define ustva-2005:export
  (lambda (buffer)
    (let ((zeitraum (string->number (storage:retrieve buffer "zeitraum")))
	  (land     (string->number (storage:retrieve buffer "land")))
	  (st-nr    (storage:retrieve buffer "stnr")))
		    
      ;; manipulate 'zeitraum', 0 => January, 11 => Dec., 12 => I/05 .. 15
      (set! zeitraum (number->string
		      (if (> zeitraum 11)
			  (+ zeitraum 29)    ; 12 + 29 = 41 => Q1 !!
			  (+ zeitraum  1)))) ; January => 01 !!
	  
      (if (= (string-length zeitraum) 1)
	  (set! zeitraum (string-append "0" zeitraum)))

    (list "Umsatzsteuervoranmeldung" #f
	  (list "Jahr"         #f "2005"
		"Zeitraum"     #f zeitraum
		"Steuernummer" #f (steuernummer:convert land st-nr)
		"Kz09"         #f (export:generate-kz09 buffer)
))
    )))
		  

    

