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



;; try to load defaults file ...
(let ((fn (string-append (getenv "HOME") "/.taxbird/datenlieferant.dat")))
  (or (and (file-exists? fn)
	   (or (load fn) #t))

      ;; create default set ...
      (define datenlieferant:defaults '())))

	   
	 
	 

(define datenlieferant:button-demux
  (lambda (buffer field)
    (if (not (string=? field "store"))
	#f

	(let ((fn (string-append (getenv "HOME") "/.taxbird/"))
	      (handle #f)
	      (fields '("anschluss" "name-lieferant" "berufsbez" 
			"ort-lieferant" "datenlieferant" "plz-lieferant"
			"strasse-lieferant" "land-lieferant" "vorwahl")))
	  (or
	   (file-exists? fn)
	   (mkdir fn)) ;; create directory ~/.taxbird/

	  ;; open file ~/.taxbird/datenlieferant.dat ...
	  (set! fn (string-append fn "datenlieferant.dat"))
	  (set! handle (open fn (logior O_WRONLY O_CREAT)))

	  ;; store defaults to the file ...
	  (format handle "(define datenlieferant:defaults '(")
	  (for-each
	   (lambda(field)
	     (format handle "(~S . ~S)" field (storage:retrieve buffer field)))
	   fields)
	  (format handle "))~%")
	  
	  (close handle)
	  (load fn)))))



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



(define datenlieferant:validators
  (list 
   "name-lieferant"
   (lambda(val buf) 
     (and (validate:datenlieferant val buf "name-lieferant")
	  (not (string-index val #\*))
	  (validate:alphanum val 1 45)))

   "strasse-lieferant"
   (lambda(value buf)
     (validate:alphanum value 1 30))

   "plz-lieferant" 
   (lambda(val buf)
     (and (validate:datenlieferant val buf "plz-lieferant")
	  (validate:alphanum val 1 12)))

   "ort-lieferant"
   (lambda(val buf)
     (and (validate:datenlieferant val buf "ort-lieferant")
	  (validate:alphanum val 1 30)))

   "land-lieferant" 
   (lambda(val buf)
     (validate:datenlieferant val buf "land-lieferant"))

   "vorwahl" 
   (lambda(val buf)
     (and (validate:datenlieferant val buf "vorwahl")
	  (not (string-index val #\*))
	  (validate:unsigned-int val buf)))

   "anschluss" 
   (lambda(val buf)
     (and (validate:datenlieferant val buf "anschluss")
	  (not (string-index val #\*))
	  (validate:unsigned-int val buf)))


   "berufsbez"
   (lambda(val buf)
     (or (not val)   ;; field may be empty ...
	 (and (not (string-index val #\*))
	      (validate:datenlieferant 
	       val buf "berufsbez"))))
   
   "mandant" 
   (lambda(val buf)
     (or (not val)  ;; empty field is okay ...
	 (and (not (string-index val #\*))
	      (validate:kz09-maxlen val buf "mandant"))))))




(define datenlieferant:validate
  (lambda (buffer element value)
    (let ((func (member element datenlieferant:validators)))
      (if (not func)
	  (let ()
	    ;;(format #t "cannot find validator for ~S~%" element)
	    #t)

	  (let ()
	    ;;(format #t "validating ~S => ~S against ~S~%"
	    ;;        element value (cadr func))
	    ((cadr func) value buffer))))))



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
