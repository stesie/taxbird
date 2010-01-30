;; Copyright(C) 2004,2005,2006,2008,2009,2010 Stefan Siegl <stesie@brokenpipe.de>
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
;;; Umsatzsteuervoranmeldung 2010                                           ;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(define ustva-2010:definition
  '("Datenlieferant" ("datenlieferant.xml" "datenlieferant")
    
    "Finanzamtsverbindung" 	("ustva-2010.xml" "stammdaten")
    "Schnellerfassung"          ("ustva-2010.xml" "schnell")
    "Formular"		        ("ustva-2010.xml" "seite_1")))




(define ustva-2010:validators
  (list 
   ;; make sure, that the choosers have a value assigned.
   "land" (lambda(val buf)
	    (and val (if (string? val) (string->number val) #t)))
   "zeitraum" (lambda(val buf)
		(and val (if (string? val) (string->number val) #t)))
   
   "stnr" steuernummer:validate

   "Kz41" validate:signed-int
   "Kz44" validate:signed-int
   "Kz49" validate:signed-int
   "Kz43" validate:signed-int
   "Kz48" validate:signed-int
   "Kz81" validate:signed-int   ;; Ums. regulär, 19% USt
   "Kz86" validate:signed-int   ;; Ums. regulär,  7% 
   "Kz35" validate:signed-int   ;; Ums. regulär, andere Steuersätze
   
   "Kz36" 
   (lambda(val buf)
     (let ((maximum (storage:retrieve buf "Kz35")))
       (validate:signed-monetary-max val buf maximum)))
   
   "Kz77" validate:signed-int
   "Kz76" validate:signed-int
   
   "Kz80" 
   (lambda (val b)
     (let ((maximum (storage:retrieve b "Kz76")))
       (validate:signed-monetary-max val b maximum)))
   
   "Kz91" validate:signed-int
   "Kz89" validate:signed-int   ;; Ums. innerg. Erw. 19% USt ab 2010
   "Kz93" validate:signed-int
   "Kz95" validate:signed-int

   "Kz98"
   (lambda(val buf)
     (let ((maximum (storage:retrieve buf "Kz95")))
       (validate:signed-monetary-max val buf maximum)))

   "Kz94" validate:signed-int

   "Kz96"
   (lambda(val buf)
     (let ((maximum (storage:retrieve buf "Kz94")))
       (validate:signed-monetary-max val buf maximum)))

   "Kz42" validate:signed-int
   "Kz60" validate:signed-int
   "Kz21" validate:signed-int
   "Kz45" validate:signed-int

   "Kz46" validate:signed-int
   "Kz47"
   (lambda (val buf)
     (let ((maximum (storage:retrieve buf "Kz46")))
       (validate:signed-monetary-max val buf maximum)))

   "Kz52" validate:signed-int
   "Kz53"
   (lambda (val buf)
     (let ((maximum (storage:retrieve buf "Kz52")))
       (validate:signed-monetary-max val buf maximum)))

   "Kz73" validate:signed-int
   "Kz74"
   (lambda (val buf)
     (let ((maximum (storage:retrieve buf "Kz73")))
       (validate:signed-monetary-max val buf maximum)))

   "Kz84" validate:signed-int
   "Kz85"
   (lambda (val buf)
     (let ((maximum (storage:retrieve buf "Kz84")))
       (validate:signed-monetary-max val buf maximum)))
   
   "Kz65" validate:signed-monetary
   "Kz66" validate:signed-monetary
   "Kz61" validate:signed-monetary
   "Kz62" validate:signed-monetary
   "Kz67" validate:signed-monetary
   "Kz63" validate:signed-monetary
   "Kz64" validate:signed-monetary
   "Kz59" validate:signed-monetary
   "Kz69" validate:signed-monetary
   "Kz39" validate:unsigned-int))




(define ustva-2010:validate
  (lambda (buffer element value)
    (let ((func (member element ustva-2010:validators)))
      (if (not func)
	  (let ()
	    ;;(format #t "cannot find validator for ~S~%" element)
	    #t)

	  (let ()
	    ;;(format #t "validating ~S => ~S against ~S~%"
	    ;;        element value (cadr func))
	    ((cadr func) value buffer))))))




(define ustva-2010:recalculate
  (lambda (buffer element value)
    (let ((list '(
		  ;; regulärer Umsatz, 19 % USt (ab 2010)
		  "Kz81" (lambda(v buffer)
			   (storage:store buffer "Kz81-calc"
					  (number->monetary-string
					   #t (/ (* (ms->number v) 19) 100))))

		  ;; regulärer Umsatz 7 % USt
		  "Kz86" (lambda(v buffer)
			   (storage:store buffer "Kz86-calc"
					  (number->monetary-string 
					   #t (/ (* (ms->number v) 7) 100))))

		  ;; innergem. Erwerb 7 % USt
		  "Kz93" (lambda(v buffer)
			   (storage:store buffer "Kz93-calc"
					  (number->monetary-string 
					   #t (/ (* (ms->number v) 7) 100))))

		  ;; innergem. Erwerb 16 % USt
		  "Kz89" (lambda(v buffer)
			   (storage:store buffer "Kz89-calc"
					  (number->monetary-string
					   #t (/ (* (ms->number v) 19) 100))))
		  )))

      ;; recurse through the upper list 'list', looking for the field to
      ;; use as the calculation base ...
      (while (> (length list) 0)
	     (if (string=? element "")
		 (let ()
		   (set! value (storage:retrieve buffer (car list)))
		   (if (or (not (string? value))
			   (= (string-length value) 0))
		       (set! value "0"))
		   ((eval (cadr list) (current-module)) value buffer))

		 (if (string=? (car list) element)
		     (let ()
		       (if (= (string-length value) 0)
			   (set! value "0"))
		       ((eval (cadr list) (current-module)) value buffer))))
	     (set! list (cddr list))))


    ;; calculate various sums in the sheets ..
    (let ((fields (list "uebertrag"  (list "Kz36" "Kz86-calc" "Kz81-calc"
			                   "Kz80" "Kz96" "Kz98" "Kz93-calc"
					   "Kz89-calc")
			"ust-sum"    (list "uebertrag" "Kz85" "Kz74" "Kz53" "Kz47"
					   "Kz65")
			"vorst-sum"  (list "Kz66" "Kz61" "Kz62" "Kz67" "Kz63"
					   "Kz64" "Kz59")
			"ust-sum+69" (list "ust-sum" "Kz69")))
	  (sum 0))
      (while (> (length fields) 0)
	     (set! sum 0)

	     ;; now calculate the sum of the listed fields
	     (for-each
	      (lambda (field)
		(let ((field-val (storage:retrieve buffer field)))
		  (if (and field-val (> (string-length field-val) 0))
		      (set! sum (+ sum (ms->number field-val))))))

	      (cadr fields))
	     
	     ;; store the result ...
	     (storage:store buffer (car fields)
			    (number->monetary-string #t sum))

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
			      (ms->number val) 0))

	     (set! val (storage:retrieve buffer (caddr fields)))
	     (if (and val (> (string-length val) 0))
		 (set! result (- result (ms->number val))))

	     ;; store the result ...
	     (storage:store buffer (car fields)
			    (number->monetary-string #t result))

	     ;; forward the list ...
	     (set! fields (cdddr fields))))))



(define ustva-2010:export
  (lambda (buffer sig-result)
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
	   (list "Jahr"         #f "2010"
		 "Zeitraum"     #f zeitraum
		 "Steuernummer" #f (steuernummer:convert land st-nr)
		 "Kz09"         #f (export:generate-kz09 buffer sig-result))

	   ;; finally copy all the Kz?? values from the buffer ...
	   ;; this can be done in any order, since libgeier will reformat
	   ;; it for us ...
	   (let ((result '())
		 (fields (list (lambda (val)
				 (format #f "~,2F" (ms->number val)))
			       (list "Kz36" "Kz39" "Kz47" "Kz53" "Kz59" "Kz61"
				     "Kz62" "Kz63" "Kz64" "Kz65" "Kz66"
				     "Kz67" "Kz69" "Kz74" "Kz80" "Kz83"
				     "Kz85" "Kz96" "Kz98")

			       (lambda (val)
				 (format #f "~D" (inexact->exact 
						  (ms->number val))))
			       (list "Kz21" "Kz35" "Kz41" "Kz42" "Kz43" "Kz44"
				     "Kz45" "Kz48" "Kz49" "Kz46" "Kz52" "Kz60"
				     "Kz76" "Kz73" "Kz77" "Kz81" "Kz84" "Kz86"
				     "Kz89" "Kz91" "Kz93" "Kz94" "Kz95"

				     ;; checkboxes ...
				     "Kz10" "Kz26" "Kz29" "Kz22"
				     ))))

	     (while (> (length fields) 0)
		    (for-each
		     (lambda (field)
		       (let ((value (storage:retrieve buffer field)))
			 (if (and value
				  (> (string-length value) 0))
			     (let ((out-val ((car fields) value)))
			       ;; don't write out fields, that are equal to
			       ;; zero, except for Kz83 which is the total
			       (if (or (string=? field "Kz83")
				       (not (= (ms->number value) 0)))
				   (set! result
					 (append result
						 (list field #f out-val))))))))
		     
		     (cadr fields))

		    ;; forward the list ..
		    (set! fields (cddr fields)))

	     result))))))
		  


(define ustva-2010:export-steuerfall
  (lambda (buffer sig-result)
    (list 
     "Anmeldungssteuern" (list (list "art" "UStVA") 
			       (list "version" "201001"))
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
	   "Steuerfall"       #f (ustva-2010:export buffer sig-result)))))



(define ustva-2010:get-sheet
  (lambda (sheet-name)
    (tb:eval-file "sheettree.scm")

    (or (get-sheet sheet-name ustva-2010:definition))))


