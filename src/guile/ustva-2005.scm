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
(tb:eval-file "basics-datenlieferant.scm")
(tb:eval-file "bundesland-chooser.scm")
(tb:eval-file "steuernummer.scm")
(tb:eval-file "validate.scm")
(tb:eval-file "zeitraum-chooser.scm")

;; we use ice-9 pretty printer to reformat our numbers ...
(use-modules (ice-9 format))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Umsatzsteuervoranmeldung 2005                                           ;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define ustva-2005:definition
  (list

   "Allgemeine Daten"

   (list

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    "Stammdaten"

    (list
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    "Datenlieferant"    basics:datenlieferant

    "Finanzamtsverbindung"
    (list 1
	  bundesland:chooser

	  (list "Steuernummer"
		tb:field:text-input
		"stnr"
		steuernummer:help
		steuernummer:validate)))

    
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    "Aktuelle Voranmeldung"

    (list 1
	  zeitraum:chooser

	  (list "Berichtigte Anmeldung"
		tb:field:checkbox
		"Kz10"
		"Berichtigte Anmeldung"
		#t)
	  
	  (list "Verrechnung des Erstattungs-\nbetrages erwünscht"
		tb:field:checkbox
		"Kz29"
		(string-append "Verrechnung des Erstattungsbetrages "
			       "erwünscht / Erstattungsbetrag ist abgetreten")
		#t)
	  

	  (list "Einzugsermächtigung wird\nausnahmsweise widerrufen"
		tb:field:checkbox
		"Kz26"
		(string-append "Die Einzugsermächtigung wird ausnahmsweise "
			       "(z.B. wegen Verrechnungswünschen) für "
			       "diesen Voranmeldungszeitraum widerrufen. "
			       "Ein ggf. verbleibender Rest ist gesondert zu "
			       "entrichten.")
		#t)))
   


   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
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
			       "(§ 2a UStG)")
		validate:signed-int)

	  
	   ;;;;;;;;;;;;;;;;;;
	  (list "<b>Weitere steuerfreie Umsätze ...</b>")


	  (list "mit Vorsteuerabzug\n(z.B. Ausfuhrlieferungen)"
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
		  (let ((maximum (storage:retrieve buf "Kz35")))
		    (validate:signed-monetary-max val buf maximum))))
						


	  (list "<i>Summe</i>"

		tb:field:label "" #f #t ; skip first column

		;; second column ...
		tb:field:text-output
		"stpfl-ums"
		#f
		#t)



	  (list "<big><b>Umsätze nach § 24 UStG</b></big>")

	  (list (string-append "Lief. übriges Gemeinschaftsgeb.,\n"
			       "an Abnehmer <b>mit</b> USt-IdNr.")
		tb:field:text-input
		"Kz77"
		(string-append "Umsätze land- und forstwirtschaftl. Betriebe "
			       "nach § 24 UStG; Lieferungen in das übrige "
			       "Gemeinschaftsgebiet an Abnehmer <b>mit</b> "
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
		  (let ((maximum (storage:retrieve b "Kz76")))
		    (validate:signed-monetary-max val b maximum))))))



   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   "Innergemeinschaftliche Erwerbe"
   (list 2

	 (list "<big><b>Steuerfreie innergem. Erwerbe ...</b></big>")

	 (list "steuerfrei, § 4b UStG"
	       tb:field:text-input
	       "Kz91" 
	       "Steuerfreie innergemeinschaftliche Erwerbe nach § 4b UStG" 
	       validate:signed-int)

	 (list (string-append "<big><b>Steuerpflichtige "
			      "innergem. Erwerbe ...</b></big>"))
	 
	 (list "zum Steuersatz von 16%"
	       tb:field:text-input
	       "Kz97"
	       (string-append "Steuerpflichtige innergemeinschaftliche "
			      "Erwerbe zum Steuersatz von 16 v.H.")
	       validate:signed-int
	       
	       ;; second field ...
	       tb:field:text-output
	       "Kz97-calc" #f #t)

	  
	 (list "zum Steuersatz von 7%"
	       tb:field:text-input
	       "Kz93"
	       (string-append "Steuerpflichtige innergemeinschaftliche "
			      "Erwerbe zum Steuersatz von 7 v.H.")
	       validate:signed-int
	       
	       ;; second field ...
	       tb:field:text-output
	       "Kz93-calc" #f #t)
	 
	 
	 (list "andere Steuersätze"
	       tb:field:text-input
	       "Kz95"
	       (string-append "Steuerpflichtige innergemeinschaftliche "
			      "Erwerbe, die anderen Steuersätzen "
			      "unterliegen (Umsatz)")
	       validate:signed-int

	       ;; second field
	       tb:field:text-input
	       "Kz98"
	       (string-append "Steuerpflichtige innergemeinschaftliche "
			      "Erwerbe, die anderen Steuersätzen "
			      "unterliegen (Steuer)")
	       (lambda(val buf)
		 (let ((maximum (storage:retrieve buf "Kz95")))
		   (validate:signed-monetary-max val buf maximum))))
	 

	 (list "<b>neuer Fahrzeuge ...</b>")

	 (list (string-append "von Lieferern <b>ohne</b> USt-IdNr.\n"
			      "zum allgemeinen Steuersatz")
	       tb:field:text-input
	       "Kz94"
	       (string-append "Steuerpflichtige innergemeinschaftliche "
			      "Erwerbe, neuer Fahrzeuge von Lieferern "
			      "<b>ohne</b> USt-IdNr. zum allgemeinen "
			      "Steuersatz (Umsatz)")
	       validate:signed-int
	       
	       ;; second field
	       tb:field:text-input
	       "Kz96"
	       (string-append "Steuerpflichtige innergemeinschaftliche "
			      "Erwerbe, neuer Fahrzeuge von Lieferern "
			      "<b>ohne</b> USt-IdNr. zum allgemeinen "
			      "Steuersatz (Steuer)")
	       (lambda(val buf)
		 (let ((maximum (storage:retrieve buf "Kz94")))
		   (validate:signed-monetary-max val buf maximum))))


	 (list "<i>Summe</i>"

	       tb:field:label "" #f #t ; skip first column

	       ;; second column ...
	       tb:field:text-output
	       "innerg-erw"
	       #f
	       #t))
   


   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   "Ergänzende Angaben zu Umsätzen"
   (list 2

	 (list "Lief. des 1. Abnehmers\nbei innergem. Dreiecksgesch."
	       tb:field:text-input
	       "Kz42"
	       (string-append "Umsätze aus Lieferungen des ersten Abnehmers "
			      "bei <b>innergemeinschaftlichen "
			      "Dreiecksgeschäften</b> (§ 25b Abs. 2 UStG)")
	       validate:signed-int)


	 (list "Steuerpfl. Umsätze § 13b UStG"
	       tb:field:text-input
	       "Kz60"
	       (string-append "Steuerpflichtige Umsätze im Sinne des § 13b "
			      "Abs. 1 Satz 1 Nr. 1 bis 5 UStG, für die der "
			      "<b>Leistungsempfänger</b> die "
			      "<b>Steuer schuldet</b>")
	       validate:signed-int)

	 
	 (list "im Inland nicht\nsteuerbare Umsätze"
	       tb:field:text-input
	       "Kz45"
	       "im Inland nicht steuerbare Umsätze"
	       validate:signed-int))



   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   "Umsätze § 13b Abs. 2 UStG"
   (list 2

	 (list "Leistungen von\nUnternehmern im Ausland"
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


	 (list "Lief. sichergsübereig. Gegenst.,\nUmsätze unter GrEStG fallend"
	       tb:field:text-input
	       "Kz73"
	       (string-append "Lieferungen sicherungsübereigneter Gegenstände "
			      "und Umsätze, die unter das GrEStG fallen "
			      "(§ 13b Abs. 1 Satz 1 Nr. 2 und 3 UStG), für "
			      "die als Leistungsempfänger Steuer geschuldet "
			      "wird - (Umsatz)")
	       validate:signed-int

	       ;; 2nd field
	       tb:field:text-input
	       "Kz74"
	       (string-append "Lieferungen sicherungsübereigneter Gegenstände "
			      "und Umsätze, die unter das GrEStG fallen "
			      "(§ 13b Abs. 1 Satz 1 Nr. 2 und 3 UStG), für "
			      "die als Leistungsempfänger Steuer geschuldet "
			      "wird - (Steuer)")
	       (lambda (val b)
		 (validate:signed-monetary-max val b
					       (storage:retrieve b "Kz73"))))


	 (list "Bauleistungen eines\nUnternehmers im Inland"
	       tb:field:text-input
	       "Kz84"
	       (string-append "Bauleistungen eines im Inland ansässigen "
			      "Unternehmers (§ 13b Abs. 1 Satz 1 Nr. 4 UStG), "
			      "für die als Leistungsempfänger Steuer "
			      "geschuldet wird - (Umsatz)")
	       validate:signed-int

	       ;; 2nd field
	       tb:field:text-input
	       "Kz85"
	       (string-append "Bauleistungen eines im Inland ansässigen "
			      "Unternehmers (§ 13b Abs. 1 Satz 1 Nr. 4 UStG), "
			      "für die als Leistungsempfänger Steuer "
			      "geschuldet wird - (Steuer)")
	       (lambda (val b)
		 (let ((maximum (storage:retrieve b "Kz84")))
		 (validate:signed-monetary-max val b maximum))))


	 (list "<i>Summe</i>"
	       tb:field:label "" #f #t ; skip first column

	       ;; second column ...
	       tb:field:text-output "13b-sum" #f #t))


   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   "Wechsel Besteuerungsform / Nachsteuer"
   (list 1
	 
	 (list (string-append "Wechsel Bestuerungsform sowie\n"
			      "Nachsteuer versteuerte Anzahlungen")
	       tb:field:text-input
	       "Kz65"
	       (string-append "Steuer infolge Wechsels der Besteuerungsform "
			      "sowie Nachsteuer auf versteuerte Anzahlungen "
			      "wegen Steuersatzerhöhung")
	       validate:signed-monetary))



   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   "Abziehbare Vorsteuerbeträge"
   (list 2

	 (list "aus Rechnungen\nanderer Unternehmer"
	       tb:field:text-input
	       "Kz66"
	       (string-append "Vorsteuerbeträge aus Rechnungen von anderen "
			      "Unternehmern (§ 15 Abs. 1 Satz 1 Nr. 1 UStG), "
			      "aus Leistungen im Sinne des § 13a Abs. 1 Nr. 6 "
			      "UStG (§ 15 Abs. 1 Satz 1 Nr. 5 UStG) und aus "
			      "innergemeinschaftlichen Dreiecksgeschäften "
			      "(§ 25b Abs. 5 UStG)")
	       validate:signed-monetary)


	 (list "Vorsteuerbeträge aus\ninnergem. Erwerb"
	       tb:field:text-input
	       "Kz61"
	       (string-append "Vorsteuerbeträge aus dem innergemeinschaft"
			      "lichen Erwerb von Gegenständen "
			      "(§ 15 Abs. 1 Satz 1 Nr. 3 UStG)")
	       validate:signed-monetary)


	 (list "Einfuhrumsatzsteuer"
	       tb:field:text-input
	       "Kz62"
	       (string-append "Entrichtete Einfuhrumsatzsteuer "
			      "(§ 15 Abs. 1 Satz 1 Nr. 2 UStG)")
	       validate:signed-monetary)


	 (list "Vorsteuer aus Leistungen\nim Sinne des § 13b UStG"
	       tb:field:text-input
	       "Kz67"
	       (string-append "Vorsteuerbeträge aus Leistungen im Sinne "
			      "des § 13b Abs. 1 UStG "
			      "(§ 15 Abs. 1 Satz 1 Nr. 4 UStG)")
	       validate:signed-monetary)


	 (list "Vorsteuer, nach allgemeinenen\nDurchschnittsätzen berechnet"
	       tb:field:text-input
	       "Kz63"
	       (string-append "Vorsteuerbeträge, die nach allgemeinen "
			      "Durchschnittsätzen berechnet sind "
			      "(§§ 23 und 23a UStG)")
	       validate:signed-monetary)


	 (list "Berichtigung des Vorsteuer-\nabzugs (§ 15a UStG)"
	       tb:field:text-input
	       "Kz64"
      	       "Berichtigung des Vorsteuerabzugs (§ 15a UStG)"
	       validate:signed-monetary)

	 
	 (list "Vorsteuerabzug für innergem.\nLieferungen neuer Fahrzeuge"
	       tb:field:text-input
	       "Kz59"
	       (string-append "Vorsteuerabzug für innergemeinschaftliche "
			      "Lieferungen neuer Fahrzeuge außerhalb eines "
			      "Unternehmens (§ 2a UStG) sowie von Klein"
			      "unternehmern im Sinne des § 19 Abs. 1 UStG "
			      "(§ 15 Abs. 4a UStG)")
	       validate:signed-monetary)

	 (list "<i>Summe</i>"
	       tb:field:text-output
	       "vorst-sum"
	       #f
	       #t))



   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   "Sonstige Angaben"
   (list 2


	 (list "Diverse Sonstige (Kz69)"
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


	 (list "Anrechnung festgesetzte\nSondervorauszahlung"
	       tb:field:text-input
	       "Kz39"
	       (string-append "Anrechnung (Abzug) der festgesetzten "
			      "Sondervorauszahlung für Dauerfristverlängerung "
			      "(nur auszufüllen in der letzten Voranmeldung "
			      "des Besteuerungszeitraums, in der Regel "
			      "Dezember)")
	       validate:unsigned-int))


   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   "Abrechnung"
   (list 1

	 (list "Umsatzsteuer"
	       tb:field:text-output "ust-sum" #f #t)

	 (list "Vorsteuerbeträge"
	       tb:field:text-output "vorst-sum" #f #t)

	 (list "Verbleibender Betrag,\nnach Abzug Vorsteuer"
	       tb:field:text-output "ust-minus-vost" #f #t)

	 ;;;(list "Umsatzsteuer-Voraus-\nzahlung/Überschuss"
	 ;;;      tb:field:text-output "ust-vz" #f #t)

	 (list "<b>Verbleibende Umsatz-\nsteuer-Vorauszahlung</b>"
	       tb:field:text-output "Kz83" #f #t))))




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

      ;; recurse through the upper list 'list', looking for the field to
      ;; use as the calculation base ...
      (while (> (length list) 0)
	     (if (string=? (car list) element)
		 (let ()
		   (if (= (string-length value) 0)
		       (set! value "0"))
		   ((eval (cadr list) (current-module)) value buffer)))
	     (set! list (cddr list))))


    ;; calculate various sums in the sheets ..
    (let ((fields (list "stpfl-ums"  (list "Kz36" "Kz86-calc" "Kz51-calc")
			"innerg-erw" (list "Kz96" "Kz98" "Kz93-calc"
					   "Kz97-calc")
			"13b-sum"    (list "Kz85" "Kz74" "Kz53")
			"vorst-sum"  (list "Kz66" "Kz61" "Kz62" "Kz67" "Kz63"
					   "Kz64" "Kz59")
			"ust-sum"    (list "stpfl-ums" "Kz80" "innerg-erw" 
					   "13b-sum" "Kz65")
			"ust-sum+69" (list "ust-sum" "Kz69")))
	  (sum 0))
      (while (> (length fields) 0)
	     (set! sum 0)

	     ;; now calculate the sum of the listed fields
	     (for-each
	      (lambda (field)
		(let ((field-val (storage:retrieve buffer field)))
		  (if (and field-val (> (string-length field-val) 0))
		      (set! sum (+ sum (string->number field-val))))))

	      (cadr fields))
	     
	     ;; store the result ...
	     (storage:store buffer (car fields) (number->string sum))

	     ;; forward the list ...
	     (set! fields (cddr fields))))



    ;; calculate various differences in the sheets ..
    (let ((fields (list "ust-minus-vost" "ust-sum" "vorst-sum"
			"ust-vz"         "ust-sum+69" "vorst-sum" ; + "Kz69"!
			"Kz83"           "ust-vz" "Kz39"))
	  (result 0) (val 0))

      (while (> (length fields) 0)
	     (set! val (storage:retrieve buffer (cadr fields)))
	     (set! result (if (and val (> (string-length val) 0))
			      (string->number val) 0))

	     (set! val (storage:retrieve buffer (caddr fields)))
	     (if (and val (> (string-length val) 0))
		 (set! result (- result (string->number val))))

	     ;; store the result ...
	     (storage:store buffer (car fields) (number->string result))

	     ;; forward the list ...
	     (set! fields (cdddr fields))))))



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
	  (append
	   (list "Jahr"         #f "2005"
		 "Zeitraum"     #f zeitraum
		 "Steuernummer" #f (steuernummer:convert land st-nr)
		 "Kz09"         #f (export:generate-kz09 buffer))

	   ;; finally copy all the Kz?? values from the buffer ...
	   ;; this can be done in any order, since libgeier will reformat
	   ;; it for us ...
	   (let ((result '())
		 (fields (list "~,2F" (list "Kz36" "Kz39" "Kz53" "Kz59" "Kz61"
					    "Kz62" "Kz63" "Kz64" "Kz65" "Kz66"
					    "Kz67" "Kz69" "Kz74" "Kz80" "Kz83"
					    "Kz85" "Kz96" "Kz98")
			       "~D"   (list "Kz35"
					    "Kz41" "Kz42" "Kz43" "Kz44" "Kz45"
					    "Kz48" "Kz49" "Kz51" "Kz52" "Kz60"
					    "Kz76" "Kz73" "Kz77" "Kz84" "Kz86"
					    "Kz91" "Kz93" "Kz94" "Kz95" "Kz97"

					    ;; checkboxes ...
					    "Kz10" "Kz26" "Kz29"
					    ))))

	     (while (> (length fields) 0)
		    (for-each
		     (lambda (field)
		       (let ((value (storage:retrieve buffer field)))
			 (if (and value
				  (> (string-length value) 0))
			     (let ((out-val (format #f (car fields)
						    (string->number value))))
			       
			       ;; don't write out fields, that are equal to
			       ;; zero, except for Kz83 which is the total
			       (if (or (string=? field "Kz83")
				       (not (= (string->number value) 0)))
				   (set! result
					 (append result
						 (list field #f out-val))))))))
		     
		     (cadr fields))

		    ;; forward the list ..
		    (set! fields (cddr fields)))

	     result))))))
		  

    

