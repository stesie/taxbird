;; Copyright(C) 2004,2005,2006,2008 Stefan Siegl <stesie@brokenpipe.de>
;; taxbird - free program to interface with German IRO's Elster/Coala
;;
;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3 of the License, or
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
(tb:eval-file "datenlieferant.scm")
(tb:eval-file "steuernummer.scm")
(tb:eval-file "validate.scm")

;; we use ice-9 pretty printer to reformat our numbers ...
(use-modules (ice-9 format))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Antrag auf USt-Dauerfristverlaengerung 2008                             ;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define ustsvza-2008:export
  (lambda (buffer sig-result)
    (let ((land  (string->number (storage:retrieve buffer "land")))
	  (st-nr (storage:retrieve buffer "stnr"))
	  (mode  (storage:retrieve buffer "Umsatzsteuersondervorauszahlung")))
		    
      (list mode #f
	    (append
	     (list "Jahr"         #f "2008"
		   "Steuernummer" #f (steuernummer:convert land st-nr)
		   "Kz09"         #f (export:generate-kz09 buffer sig-result))

	     ;; finally copy all the Kz?? values from the buffer ...
	     ;; this can be done in any order, since libgeier will reformat
	     ;; it for us ...
	     (let ((result '())
		   (fields (list (lambda (val)
				   (format #f "~D" (inexact->exact 
						    (ms->number val))))

				 (if (string=? mode "Dauerfristverlaengerung")
				     '()
				     (list "Kz38" 

					   ;; checkboxes ...
					   "Kz10" "Kz26" "Kz29")))))


	       (while (> (length fields) 0)
		      (for-each
		       (lambda (field)
			 (let ((value (storage:retrieve buffer field)))
			   (if (and value
				    (> (string-length value) 0))
			       (let ((out-val ((car fields) value)))
				 ;; don't write out fields, that are equal to
				 ;; zero, except for Kz38 which is required
				 (if (or (string=? field "Kz38")
					 (not (= (ms->number value) 0)))
				     (set! result
					   (append result
						   (list field #f out-val))))))))
		     
		       (cadr fields))

		      ;; forward the list ..
		      (set! fields (cddr fields)))
	       
	       result))))))
		  


(define ustsvza-2008:export-steuerfall
  (lambda (buffer sig-result)
    (list 
     "Anmeldungssteuern" (list (list "art" "UStVA") 
			       (list "version" "200801"))
     (list "DatenLieferant"   #f 
	   (append  
	    (list "Name"    #f (storage:retrieve buffer "name-lieferant")
		  "Strasse" #f (storage:retrieve buffer "strasse-lieferant")
		  "PLZ"     #f (storage:retrieve buffer "plz-lieferant")
		  "Ort"     #f (storage:retrieve buffer "ort-lieferant"))

	    (let ((v (storage:retrieve buffer "vorwahl"))
		  (w (storage:retrieve buffer "anschluss")))
	      (if (and v w)
		  (list "Telefon" #f (string-append v "/" w))
		  (list)))

	    (let ((v (storage:retrieve buffer "email")))
	      (if v
		  (list "Email" #f v)
		  (list))))


	   "Erstellungsdatum" #f (strftime "%Y%m%d"
					   (localtime (current-time)))
	   "Steuerfall"       #f (ustsvza-2008:export buffer sig-result)))))



