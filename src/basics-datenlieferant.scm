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
  '(tb:field:text-output
    '("vend-id" "Hersteller-ID"
      "Von der Oberfinanzdirektion München vergebene Hersteller-ID" #t)

    tb:field:text-input
    '("name-lieferant" "Name"
      "Name des Datenlieferanten (ggf. Steuerberater)"
      (lambda(val buf) (validate:datenlieferant val buf "name-lieferant")))
    
    tb:field:text-input
    '("berufsbez" "Berufsbezeichnung"
      "Berufsbezeichnung des Beraters"
      (lambda(val buf) (validate:datenlieferant val buf "berufsbez")))
    
    tb:field:text-input
    '("vorwahl" "Tel.-Nr. (Vorwahl)"
      "Tel.-Nr. Datenlieferant/Berater (Vorwahl)"
      (lambda(val buf) (validate:datenlieferant val buf "vorwahl")))

    tb:field:text-input
    '("anschluss" "Tel.-Nr. (Anschluss)"
      "Tel.-Nr. Datenlieferant/Berater (Anschluss)"
      (lambda(val buf) (validate:datenlieferant val buf "anschluss")))

    tb:field:text-input
    '("plz-lieferant" "Postleitzahl" "PLZ des Datenlieferanten" #t)

    tb:field:text-input
    '("ort-lieferant" "Ort" "Sitzort des Datenlieferanten" #t)

    tb:field:text-input
    '("land-lieferant" "Land" "Land des Datenlieferanten" #t)

    tb:field:text-input
    '("mandant" "Name Mandant"
      "Name des Mandanten (optional, sofern zutreffend aber erwünscht)"
      (lambda(val buf) (validate:datenlieferant val buf "mandant")))))


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
