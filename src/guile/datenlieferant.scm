;;               -*- mode: Scheme; coding: utf-8 -*-
;;
;; Copyright(C) 2005 Stefan Siegl <ssiegl@gmx.de>
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

(define validate:datenlieferant
  (lambda (value buffer field)
    (let ((berufsbez (if (string=? field "berufsbez") value
			 (storage:retrieve buffer "berufsbez"))))

      ;;(format #t "validate:datenlieferant F:~S, V:~S, B:~S~%"
      ;;        field value berufsbez)

      (and (or (not (and (string? berufsbez)
			 (> (string-length berufsbez) 0)))

	       ;; berufsbez. set, therefore we'll try to generate a full kz09
	       (validate:kz09-maxlen value buffer field))

	   (or (string=? field "berufsbez")

	       (and 
		(string? value) ;;; in other case, make sure we've got a 
	                        ;;; string associated, that's at least one
	                        ;;; character long.
		(> (string-length value) 0)))

	   #t)))) ;; make sure we definitely return #t if valid ...



(define validate:kz09-maxlen
  (lambda (value buffer field)
    ;; all the fields in 'datenlieferant' sheet together must not be longer
    ;; the 90 chars ..
    (let ((totallen 0)
	  (fields '("vend-id" "name-lieferant" "berufsbez" "vorwahl"
		    "anschluss" "mandant")))

      (while (> (length fields) 0)
	     (let ((field-val (if (string=? (car fields) field) value
				  (storage:retrieve buffer (car fields)))))

	       ;;;(format #t "length of field ~S: ~S~%" 
	       ;;;        (car fields) (string-length field-val))

	       (if (string? field-val)
		   (set! totallen
			 (+ totallen (string-length field-val)))))
	     (set! fields (cdr fields)))

      ;;(format #t "length of fields together: ~S~%" totallen)      
      (if (> totallen 90) #f #t))))



(define basics:datenlieferant
  (list 2

	;;(list "Hersteller-ID"
	;;      tb:field:text-output
	;;      "vend-id" 
	;;      "Von der Oberfinanzdirektion München vergebene Hersteller-ID"
	;;      #t)

	(list "<b>Adressdaten ...</b>")

	(list "Name"
	      tb:field:text-input
	      "name-lieferant"
	      "Name des Datenlieferanten (ggf. Steuerberater)"
	      (lambda(val buf) 
		(and (validate:datenlieferant val buf "name-lieferant")
		     (not (string-index val #\*)) ;; because of Kz09 field!
		     (validate:alphanum val 1 45))))

	(list "Straße" 
	      tb:field:text-input
	      "strasse-lieferant"
	      (string-append "Straßenname und Hausnummer "
			     "(der Anschrift des Datenlieferanten)")
	      (lambda(value buf)
		(validate:alphanum value 1 30)))

	(list "Postleitzahl"
	      tb:field:text-input
	      "plz-lieferant" 
	      "PLZ des Datenlieferanten" 
	      (lambda(val buf)
		(and (validate:datenlieferant val buf "plz-lieferant")
		     (validate:alphanum val 1 12))))

	(list "Ort"
	      tb:field:text-input
	      "ort-lieferant"
	      "Sitzort des Datenlieferanten" 
	      (lambda(val buf)
		(and (validate:datenlieferant val buf "ort-lieferant")
		     (validate:alphanum val 1 30))))

	(list "Land" 
	      tb:field:text-input
	      "land-lieferant" 
	      "Land des Datenlieferanten (Staat)"
	      (lambda(val buf)
		(validate:datenlieferant val buf "land-lieferant")))


	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	(list "<b>Weitere Kommunikationswege ...</b>")

	(list "Tel.-Nr. (Vorwahl)"
	      tb:field:text-input
	      "vorwahl" 
	      "Tel.-Nr. des Datenlieferanten bzw. Beraters (Vorwahl)"
	      (lambda(val buf)
		(and (validate:datenlieferant val buf "vorwahl")
		     (not (string-index val #\*)) ;; because of Kz09 field!
		     (validate:unsigned-int val buf))))

	(list "Tel.-Nr. (Anschluss)"
	      tb:field:text-input
	      "anschluss" 
	      "Tel.-Nr. des Datenlieferanten bzw. Beraters (Anschluss)"
	      (lambda(val buf)
		(and (validate:datenlieferant val buf "anschluss")
		     (not (string-index val #\*)) ;; because of Kz09 field!
		     (validate:unsigned-int val buf))))


	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	(list "<b>Weitere Angaben bei Mandatsverhältnis ...</b>")

	(list "Berufsbezeichnung\ndes Beraters"
	      tb:field:text-input
	      "berufsbez"
	      "Berufsbezeichnung des Steuerberaters (falls zutreffend)"
	      (lambda(val buf)
		(and (not (string-index val #\*)) ;; because of Kz09 field!
		     (validate:datenlieferant val buf "berufsbez"))))
    
	(list "Name Mandant"
	      tb:field:text-input
	      "mandant" 
	      "Name des Mandanten (optional, sofern zutreffend aber erwünscht)"
	      (lambda(val buf)
		(and (not (string-index val #\*)) ;; because of Kz09 field!
		     (validate:kz09-maxlen val buf "mandant"))))))


(define export:generate-datenlieferant
  (lambda (store)
    (list "DatenLieferant" #f
	  (string-append (storage:retrieve store "name-lieferant") ", "
			 (storage:retrieve store "vorwahl") "/"
			 (storage:retrieve store "anschluss") ", "
			 (storage:retrieve store "ort-lieferant") ", "
			 (storage:retrieve store "plz-lieferant") ", "
			 (storage:retrieve store "land-lieferant")))))



(define export:generate-kz09
  (lambda (store sig-result)
    (let ((berufsbez (storage:retrieve store "berufsbez"))
	  (mandant   (storage:retrieve store "mandant"))
	  (vendor-id (if (list? sig-result) (car sig-result) "74931")))

      (if (and berufsbez
	       (> (string-length berufsbez) 0))

	  ;; berufsbez. is set, i.e. there's some advisor, that files this
	  ;; tax declaration -> write out kz09 tag ...
	  (string-append vendor-id "*"
			 (storage:retrieve store "name-lieferant") "*"
			 berufsbez "*"
			 (storage:retrieve store "vorwahl") "*"
			 (storage:retrieve store "anschluss") "*"
			 (if mandant mandant ""))

	  ;; berufsbez. is not set - as it shall be set only for tax 
	  ;; advisors etc., we can assume the person who sends the data
	  ;; is the same as the person who has to pay, i.e. don't fill kz09
	  vendor-id))))
