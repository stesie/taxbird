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

   (list tb:field:chooser
	 bundesland:chooser


	 tb:field:text-input
	 (list "stnr"
	       "Steuernummer"
	       (string-append "Die vom zuständigen Finanzamt vergebene "
			      "Steuernummer. Eingabe mit Schrägstrichen.")
	       steuernummer:validate)


	 tb:field:chooser
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
    (list



     ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
     "mit Vorsteuerabzug"

     (list
      "Innerg. Lieferungen"

      (list tb:field:text-input
	    (list "Kz41"
		  "Abnehmer mit USt-ID"
		  (string-append "Innergem. Lieferungen (§ 4 Nr. 1 Buchst. b "
				 "UStG) an Abnehmer mit USt-IdNr.")
		  validate:signed-int)


	    tb:field:text-input
	    (list "Kz44"
		  "Abnehmer ohne USt-ID (Fahrzeuge)"
		  (string-append "Innergem. Lieferungen neuer Fahrzeuge an "
				 "Abnehmer ohne Ust-IdNr.")
		  validate:signed-int)


	    tb:field:text-input
	    (list "Kz49"
		  "Außerhalb eines Unternehmens (Fahrzeuge)"
		  (string-append "Innergemeinschaftliche Lieferungen neuer "
				 "Fahrzeuge außerhalb eines Unternehmens "
				 "§ 2a UStG")
		  validate:signed-int))



     ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
      "Weitere steuerfreie Umsätze"
      (list tb:field:text-input
	    (list "Kz43"
		  "mit Vorsteuerabzug"
		  (string-append "Weitere steuerfreie Umsätze mit Vorsteuer "
				 "abzug (z.B. Ausfuhrlieferungen, Umsätze nach "
				 "§ 4 Nr. 2 bis 7 UStG)")
		  validate:signed-int)))


	
     ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
     "ohne Vorsteuerabzug"
     (list tb:field:text-input
	   (list "Kz48"
		 "Umsätze nach § 4 Nr. 8 bis 28 UStG"
		 (string-append "Umsätze nach § 4 Nr. 8 bis 28 UStG "
				"(steuerfreie Umsätze ohne Vorsteuerabzug)")
		 validate:signed-int)))



    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    "Steuerpflichtige Umsätze"
    (list tb:field:text-input-calc
	  (list "Kz51"
		"zum Steuersatz von 16%"
		(string-append "Lieferungen und sonstige Leistungen einschl. "
			       "unentgeltlicher Wertabgaben "
			       "zum Steuersatz von 16 v.H.")
		validate:signed-int

		;; second field ...
		"Kz51-calc")

	  
	  tb:field:text-input-calc
	  (list "Kz86"
		"zum Steuersatz von 7%"
		(string-append "Lieferungen und sonstige Leistungen einschl. "
			       "unentgeltlicher Wertabgaben "
			       "zum Steuersatz von 7 v.H.")
		validate:signed-int

		;; second field ...
		"Kz86-calc")


	  tb:field:text-input-input
	  (list "Kz35"
		"andere Steuersätze"
		(string-append "Lieferungen und sonstige Leistungen einschl. "
			       "unentgeltlicher Wertabgaben, Umsätze, die "
			       "anderen Steuersätzen unterliegen (Umsatz)")
		validate:signed-int

		;; second field
		"Kz36"
		#f ; no desc for 2nd field ...
		"Umsätze, die anderen Steuersätzen unterliegen (Steuer)"
		(lambda(val buf)
		  (validate:signed-monetary-max val buf
						(storage:retrieve buf
								  "Kz35")))))



    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    "Umsätze nach § 24 UStG"
    (list tb:field:text-input
	  (list "Kz77"
		"Lief. übriges Gemeinschaftsgeb., Abn. mit USt-IdNr."
		(string-append "Umsätze land- und forstwirtschaftl. Betriebe "
			       "nach § 24 UStG; Lieferungen in das übrige "
			       "Gemeinschaftsgebiet an Abnehmer *mit* "
			       "USt-IdNr.")
		validate:signed-int)


	  tb:field:text-input-input
	  (list "Kz76"
		"Umsätze, Steuer nach § 24 UStG"
		(string-append "Umsätze land- und forstwirtschaftl. Betriebe "
			       "nach § 24 UStG; Umsätze für die eine Steuer "
			       "nach § 24 UStG zu entrichten ist (Sägewerks"
			       "erzeugnisse, Getränke und alkohol. "
			       "Flüssigkeiten, z. B. Wein) - Umsatz")
		validate:signed-int

		;; second field ...
		"Kz80"
		#f ; no label ther ...
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
   (list tb:field:text-input
	 (list "Kz91" 
	       "steuerfrei, § 4b UStG"
	       "Steuerfreie innergemeinschaftliche Erwerbe nach § 4b UStG" 
	       validate:signed-int)
	 
	 
	 tb:field:text-input-calc
	 (list "Kz97"
	       "zum Steuersatz von 16%"
	       (string-append "Steuerpflichtige innergemeinschaftliche Erwerbe "
			      "zum Steuersatz von 16 v.H.")
	       validate:signed-int
	       
	       ;; second field ...
	       "Kz97-calc")

	  
	 tb:field:text-input-calc
	 (list "Kz93"
	       "zum Steuersatz von 7%"
	       (string-append "Steuerpflichtige innergemeinschaftliche Erwerbe "
			      "zum Steuersatz von 7 v.H.")
	       validate:signed-int
	       
	       ;; second field ...
	       "Kz93-calc")
	 
	 
	 tb:field:text-input-input
	 (list "Kz95"
	       "andere Steuersätze"
	       (string-append "Steuerpflichtige innergemeinschaftliche Erwerbe,"
			      " die anderen Steuersätzen unterliegen (Umsatz)")
	       validate:signed-int

	       ;; second field
	       "Kz98"
	       #f ; no desc for 2nd field ...
	       (string-append "Steuerpflichtige innergemeinschaftliche Erwerbe,"
			      " die anderen Steuersätzen unterliegen (Steuer)")
	       (lambda(val buf)
		 (validate:signed-monetary-max val buf
					       (storage:retrieve buf "Kz95"))))
	 

	 tb:field:text-input-input
	 (list "Kz94"
	       "neue Fahrz., ohne USt-IdNr."
	       (string-append "Steuerpflichtige innergemeinschaftliche Erwerbe,"
			      " neuer Fahrzeuge von Lieferern *ohne* USt-IdNr. "
			      "zum allgemeinen Steuersatz (Umsatz)")
	       validate:signed-int
	       
	       ;; second field
	       "Kz96"
	       #f ; no desc for 2nd field ...
	       (string-append "Steuerpflichtige innergemeinschaftliche Erwerbe"
			      ", neuer Fahrzeuge von Lieferern *ohne* "
			      "USt-IdNr. zum allgemeinen Steuersatz (Steuer)")
	       (lambda(val buf)
		 (validate:signed-monetary-max val buf
					       (storage:retrieve buf
								 "Kz94")))))


   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   "Ergänzende Angaben zu Umsätzen"
   (list tb:field:text-input
	 (list "Kz42"
	       "Lief. d. 1. Abn. bei innerg. Dreiecksgesch."
	       (string-append "Umsätze aus Lieferungen des ersten Abnehmers "
			      "bei *innergemeinschaftlichen Dreiecksgeschäften*"
			      " (§ 25b Abs. 2 UStG)")
	       validate:signed-int)


	 tb:field:text-input
	 (list "Kz60"
	       "Steuerpfl. Umsätze § 13b UStG"
	       (string-append "Steuerpflichtige Umsätze im Sinne des § 13b "
			      "Abs. 1 Satz 1 Nr. 1 bis 5 UStG, für die der "
			      "*Leistungsempfänger* die *Steuer schuldet*")
	       validate:signed-int)

	 
	 tb:field:text-input
	 (list "Kz45"
	       "im Inland nicht steuerbare Umsätze"
	       "im Inland nicht steuerbare Umsätze"
	       validate:signed-int))



   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   "Umsätze § 13b Abs. 2 UStG"
   (list tb:field:text-input-input
	 (list "Kz52"
	       "Leistg. v. Untern. im Ausland"
	       (string-append "Leistungen eines im Ausland ansässigen "
			      "Unternehmers (§ 13b Abs. 1 Satz 1 Nr. 1 und 5 "
			      "UStG), für die als Leistungsempfänger "
			      "Steuer geschuldet wird - Umsatz")
	       validate:signed-int

	       ;; 2nd field
	       "Kz53"
	       #f ;; no label
	       (string-append "Leistungen eines im Ausland ansässigen "
			      "Unternehmers (§ 13b Abs. 1 Satz 1 Nr. 1 und 5 "
			      "UStG), für die als Leistungsempfänger "
			      "Steuer geschuldet wird - Steuer")
	       (lambda (val b)
		 (validate:signed-monetary-max val b
					       (storage:retrieve b "Kz52"))))


	 tb:field:text-input-input
	 (list "Kz73"
	       "Lief. sichergsübereig. Gegenst., Ums. GrEStG"
	       (string-append "Lieferungen sicherungsübereigneter Gegenstände "
			      "und Umsätze, die unter das GrEStG fallen "
			      "(§ 13b Abs. 1 Satz 1 Nr. 2 und 3 UStG), für die "
			      "als Leistungsempfänger Steuer geschuldet "
			      "wird - (Umsatz)")
	       validate:signed-int

	       ;; 2nd field
	       "Kz74"
	       #f ;; no label
	       (string-append "Lieferungen sicherungsübereigneter Gegenstände "
			      "und Umsätze, die unter das GrEStG fallen "
			      "(§ 13b Abs. 1 Satz 1 Nr. 2 und 3 UStG), für die "
			      "als Leistungsempfänger Steuer geschuldet "
			      "wird - (Steuer)")
	       (lambda (val b)
		 (validate:signed-monetary-max val b
					       (storage:retrieve b "Kz73"))))


	 tb:field:text-input-input
	 (list "Kz84"
	       "Bauleistungen eines Untern. im Inland"
	       (string-append "Bauleistungen eines im Inland ansässigen "
			      "Unternehmers (§ 13b Abs. 1 Satz 1 Nr. 4 UStG), "
			      "für die als Leistungsempfänger Steuer geschuldet"
			      " wird - (Umsatz)")
	       validate:signed-int

	       ;; 2nd field
	       "Kz85"
	       #f ;; no label
	       (string-append "Bauleistungen eines im Inland ansässigen "
			      "Unternehmers (§ 13b Abs. 1 Satz 1 Nr. 4 UStG), "
			      "für die als Leistungsempfänger Steuer geschuldet"
			      " wird - (Steuer)")
	       (lambda (val b)
		 (validate:signed-monetary-max val b
					       (storage:retrieve b "Kz84")))))

   "Abziehbare Vorsteuerbeträge"
   (list tb:field:text-input
	 (list "Kz66"
	       "aus Rechnungen anderer Untern."
	       (string-append "Vorsteuerbeträge aus Rechnungen von anderen "
			      "Unternehmern (§ 15 Abs. 1 Satz 1 Nr. 1 UStG), "
			      "aus Leistungen im Sinne des § 13a Abs. 1 Nr. 6 "
			      "UStG (§ 15 Abs. 1 Satz 1 Nr. 5 UStG) und aus "
			      "innergemeinschaftlichen Dreiecksgeschäften "
			      "(§ 25b Abs. 5 UStG)")
	       validate:signed-monetary)


	 tb:field:text-input
	 (list "Kz61"
	       "Vorst. aus innergem. Erwerb"
	       (string-append "Vorsteuerbeträge aus dem innergemeinschaftlichen"
			      " Erwerb von Gegenständen (§ 15 Abs. 1 Satz 1 "
			      "Nr. 3 UStG)")
	       validate:signed-monetary)


	 tb:field:text-input
	 (list "Kz62"
	       "Einfuhrumsatzsteuer"
	       (string-append "Entrichtete Einfuhrumsatzsteuer "
			      "(§ 15 Abs. 1 Satz 1 Nr. 2 UStG)")
	       validate:signed-monetary)


	 tb:field:text-input
	 (list "Kz67"
	       "Vorsteuer § 13b"
	       (string-append "Vorsteuerbeträge aus Leistungen im Sinne "
			      "des § 13b Abs. 1 UStG "
			      "(§ 15 Abs. 1 Satz 1 Nr. 4 UStG)")
	       validate:signed-monetary)


	 tb:field:text-input
	 (list "Kz63"
	       "nach allg. Durchschnittsätze"
	       (string-append "Vorsteuerbeträge, die nach allgemeinen "
			      "Durchschnittsätzen berechnet sind "
			      "(§§ 23 und 23a UStG)")
	       validate:signed-monetary)


	 tb:field:text-input
	 (list "Kz64"
	       "Berichtigung des Vorsteuerabzugs"
	       "Berichtigung des Vorsteuerabzugs (§ 15a UStG)"
	       validate:signed-monetary)

	 
	 tb:field:text-input
	 (list "Kz59"
	       "innerg. Lief. neuer Fahrz."
	       (string-append "Vorsteuerabzug für innergemeinschaftliche "
			      "Lieferungen neuer Fahrzeuge außerhalb eines "
			      "Unternehmens (§ 2a UStG) sowie von Klein"
			      "unternehmern im Sinne des § 19 Abs. 1 UStG "
			      "(§ 15 Abs. 4a UStG)")
	       validate:signed-monetary))

   "Sonstige"
   (list tb:field:text-input
	 (list "Kz65"
	       "Wechsel Bestuerungsform sowie Nachsteuer"
	       (string-append "Steuer infolge Wechsels der Besteuerungsform "
			      "sowie Nachsteuer auf versteuerte Anzahlungen "
			      "wegen Steuersatzerhöhung")
	       validate:signed-monetary)


	 tb:field:text-input
	 (list "Kz69"
	       "Sonstiges"
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


	 tb:field:text-input
	 (list "Kz39"
	       "Anrechnung Sonder-VZ"
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
		  

    

