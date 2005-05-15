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

(define validate:alphanum
  (lambda (value min max)
    (let ((length (if (string? value) (string-length value) 0)))
      (if (< length min)
	  #f ;; length < min -> invalid

	  (if (> length max)
	      #f ;; length > max -> invalid

	      ;; FIXME make sure there are only alpha-numerical chars ...
	      #t)))))
	  

;; make sure the specified value is a signed integer
(define validate:signed-int
  (lambda (value buffer)
    (if (or (not (string? value)) (= (string-length value) 0))
	#t ; empty 
	
	(let ((conv-val (string->number value)))
	  (if (number? conv-val)
	      (if (integer? conv-val)
		  #t
		  #f)
	      #f)))))


(define validate:unsigned-int
  (lambda (value buffer)
    (if (or (not (string? value)) (= (string-length value) 0))
	#t ; empty

	(if (not (validate:signed-int value buffer))
	    #f

	    (if (< (string->number value) 0)
		#f
		#t)))))


;; make sure the specified value is a signed monetary value, i.e. not more 
;; than two cent digits
(define validate:signed-monetary
  (lambda (value buffer)
    (if (or (not (string? value)) (= (string-length value) 0))
	#t ; empty => zero

	(let ((conv-val (string->number value)))
	  (if (not conv-val)
	      #f ; NaN => error

	      (let ((split-val (string-split value #\.))
		    (valid #f))

		;; if there is no comma, it's alright ...
		(if (= (length split-val) 1)
		    (set! valid #t))

		;; if there is _one_ comma, it might be okay ...
		(if (= (length split-val) 2)
		    ;; ... if the second string is not more than 2 digits long
		    (if (<= (string-length (cadr split-val)) 2)
			(set! valid #t)))

		valid))))))



;; validation function for fields where you can enter the turnover and the
;; tax on that turnover. where 'max' is the turnover, buf the storage buffer
;; and 'val' the tax for the turnover as specified
;;
;; allowed values are (max == val == 0) and (val != 0 && val < max && max != 0)
(define validate:signed-monetary-max
  (lambda (val buf max)
    (if (string? max) (set! max (string->number max)))
    (if (not max) (set! max 0)) ;;; max may be #f, if it is either not yet
                                ;;; stored or was not a number, i.e. "" ...
    
    (if (not (validate:signed-monetary val buf))
	#f

	(let ()
	  (set! val (if (or (not (string? val))
			    (= (string-length val) 0)) "0" val))
	  (set! val (string->number val))

	  (if (= max 0)
	      ;; if max is set zero (or empty), don't allow associated field
	      ;; to have a value assigned (i.e. force it to zero)
	      (if (= val 0) #t #f)

	      ;; otherwise, if max is set, don't allow zero and force 
	      ;; value to be less than 'max', since the tax you have to pay
	      ;; for something you earned, shouldn't be 100% ;-)
	      (if (and (< val max) (not (= val 0))) #t #f))))))
