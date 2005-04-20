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
    ;; all the fields in 'datenlieferant' sheet together must not be longer
    ;; the 90 chars ..
    (let ((totallen (string-length value))
	  (fields '("vend-id" "name-lieferant" "berufsbez" "vorwahl"
		    "anschluss" "mandant")))

      (while (> (length fields) 0)
	     (if (not (string=? (car fields) field))
		 (let ((field-val (storage:retrieve buffer (car fields))))
		   (if (string? field-val)
		       (set! totallen
			     (+ totallen (string-length field-val))))))
	     (set! fields (cdr fields)))

      ;;(display "length of fields together: ") (display totallen) (newline)
      (if (> totallen 90) #f #t))))



(define basics:datenlieferant
  (list 1

	(list "Hersteller-ID"
	      tb:field:text-output
	      "vend-id" 
	      "Von der Oberfinanzdirektion München vergebene Hersteller-ID"
	      #t)

	(list "Name"
	      tb:field:text-input
	      "name-lieferant"
	      "Name des Datenlieferanten (ggf. Steuerberater)"
	      (lambda(val buf) 
		(validate:datenlieferant val buf "name-lieferant")))
    
	(list "Berufsbezeichnung"
	      tb:field:text-input
	      "berufsbez"
	      "Berufsbezeichnung des Beraters"
	      (lambda(val buf)
		(validate:datenlieferant val buf "berufsbez")))
    
	(list "Tel.-Nr. (Vorwahl)"
	      tb:field:text-input
	      "vorwahl" 
	      "Tel.-Nr. Datenlieferant/Berater (Vorwahl)"
	      (lambda(val buf)
		(validate:datenlieferant val buf "vorwahl")))

	(list "Tel.-Nr. (Anschluss)"
	      tb:field:text-input
	      "anschluss" 
	      "Tel.-Nr. Datenlieferant/Berater (Anschluss)"
	      (lambda(val buf)
		(validate:datenlieferant val buf "anschluss")))

	(list "Postleitzahl"
	      tb:field:text-input
	      "plz-lieferant" 
	      "PLZ des Datenlieferanten" 
	      #t) ;; FIXME, validation!!

	(list "Ort"
	      tb:field:text-input
	      "ort-lieferant"
	      "Sitzort des Datenlieferanten" 
	      #t) ;; FIXME, validation!!

	(list "Land" 
	      tb:field:text-input
	      "land-lieferant" 
	      "Land des Datenlieferanten"
	      #t) ;; FIXME, validation

	(list "Name Mandant"
	      tb:field:text-input
	      "mandant" 
	      "Name des Mandanten (optional, sofern zutreffend aber erwünscht)"
	      (lambda(val buf)
		(validate:datenlieferant val buf "mandant")))))


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
  (lambda (store)
    (string-append (storage:retrieve store "vend-id") "*"
		   (storage:retrieve store "name-lieferant") "*"
		   (storage:retrieve store "berufsbez") "*"
		   (storage:retrieve store "vorwahl") "*"
		   (storage:retrieve store "anschluss") "*"
		   (storage:retrieve store "mandant"))))
