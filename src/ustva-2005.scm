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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Umsatzsteuervoranmeldung 2005                                           ;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
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

    ;; Vorderseite der Umsatzsteuervoranmeldung
    "Lieferungen und sonstige Leistungen" ustva-2005:lief-und-so-leistg

))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Lieferungen und sonstige Leistungen                                     ;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define ustva-2005:lief-und-so-leistg
  '("Steuerfreie Umsätze" '("mit Vorsteuerabzug" ustva-2005:stfr-ums-vost
			    "ohne Vorsteuerabzug" ustva-2005:stfr-ums-ohne-vost)
    
    "Steuerpflichtige Umsätze" ustva-2005:stpfl-ums
;    "Umsätze nach § 24 UStG" ustva-2005:stpfl-ums-luf
))

(define ustva-2005:stfr-ums-vost
  '("Innerg. Lieferungen" '(tb:field:text-input
			    '("Kz41" "Abnehmer mit USt-ID"
			      (string-append "Innergemeinschaftliche "
					     "Lieferungen (§ 4 Nr. 1 Buchst. b "
					     "UStG) an Abnehmer mit USt-IdNr.")
			      validate:signed-int)

			    tb:field:text-input
			    '("Kz44" "Abnehmer ohne USt-ID (Fahrzeuge)"
			      (string-append "Innergemeinschaftliche "
					     "Lieferungen neuer Fahrzeuge an "
					     "Abnehmer ohne Ust-IdNr.")
			      validate:signed-int)

			    tb:field:text-input
			    '("Kz49" "Außerhalb eines Untern. (Fahrzeuge)"
			      (string-append "Innergemeinschaftliche "
					     "Lieferungen neuer Fahrzeuge "
					     "außerhalb eines Unternehmens "
					     "§ 2a UStG")
			      validate:signed-int))

    "Weitere steuerfreie Umsätze" '(tb:field:text-input
				    '("Kz43" "mit Vorsteuerabzug"
				      (string-append "Weitere steuerfreie "
						     "Umsätze mit Vorsteuer"
						     "abzug (z.B. Ausfuhrliefer"
						     "ungen, Umsätze nach § 4 "
						     "Nr. 2 bis 7 UStG)")
				      validate:signed-int))))

(define ustva-2005:stfr-ums-ohne-vost
  '(tb:field:text-input
    '("Kz48" "Umsätze nach § 4 Nr. 8 bis 28 UStG"
      (string-append "Umsätze nach § 4 Nr. 8 bis 28 UStG "
		     "(steuerfreie Umsätze ohne Vorsteuerabzug)")
      validate:signed-int)))
			  
(define ustva-2005:stpfl-ums
  '(tb:field:text-input-calc
    '("Kz51" "zum Steuersatz von 16%"
      (string-append "Lieferungen und sonstige Leistungen einschl. "
		     "unentgeltlicher Wertabgaben zum Steuersatz von 16 v.H.")
      validate:signed-int "Kz51-calc")

    tb:field:text-input-calc
    '("Kz86" "zum Steuersatz von 7%"
      (string-append "Lieferungen und sonstige Leistungen einschl. "
		     "unentgeltlicher Wertabgaben zum Steuersatz von 7 v.H.")
      validate:signed-int "Kz86-calc")

    tb:field:text-input-input
    '("Kz35" "andere Steuersätze"
      (string-append "Lieferungen und sonstige Leistungen einschl. "
		     "unentgeltlicher Wertabgaben, Umsätze, die anderen "
		     "Steuersätzen unterliegen (Umsatz)")
      validate:signed-int
      "Kz36" #f "Umsätze, die anderen Steuersätzen unterliegen (Steuer)"
      (lambda(val buf)
	(if (validate:signed-monetary val buf)
	    (let ((kz35val (storage:retrieve buf "Kz35")))
	      (if (= (string-length val) 0) (set! val "0"))
	      (if (or (not (string? kz35val)) (= (string-length kz35val) 0))
		  (set! kz35val "0"))
	      (if (< (string->number val) (string->number kz35val)) #t #f))
	    #f)))))



(define ustva-2005:recalculate
  (lambda (buffer element value)
    (let ((list '("Kz51" (lambda(v buffer)
			   (storage:store buffer "Kz51-calc"
					  (/ (* (string->number v) 16) 100)))
		  "Kz86" (lambda(v buffer)
			   (storage:store buffer "Kz86-calc"
					  (/ (* (string->number v) 7) 100))))))

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
		  

    

(tb:form-register
 ; form's name and definition
 "Umsatzsteuervoranmeldung 2005" ustva-2005:definition

 ; retrieval function
 storage:retrieve

 ; storage function
 (lambda (buffer element value)
   (storage:store buffer element value)
   (ustva-2005:recalculate buffer element value))

 ; export function
 (lambda (buf)
   (tb:eval-file "export.scm")
   (export:make-elster-xml
    (export:make-transfer-header buf "UStVA" #t)
    (export:make-nutzdaten-header buf)
    (export:make-steuerfall buf "UStVA" "200501" (ustva-2005:export buf))))

 ; empty set
 (lambda () '(("vend-id" . "74931"))))

