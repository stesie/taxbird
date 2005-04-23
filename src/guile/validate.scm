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
;; FIXME: 123.00000000001 will be accepted, perhaps do some text-based scanning
;;        also! The question is whether anybody cares ;-)
(define validate:signed-monetary
  (lambda (value buffer)
    (if (or (not (string? value)) (= (string-length value) 0))
	#t ; empty => zero

	(let ((conv-val (string->number value)))
	  (if (not conv-val)
	      #f ; NaN => error

	      (let ((conv-val (* 100 conv-val)) (diff #f))
		(set! diff (- conv-val (floor conv-val)))

		(if (< diff 1e-10) ; due to float pt. arith. we need this :(
		    #t
		    #f)))))))



(define validate:signed-monetary-max
  (lambda (val buf max)
    (if (not (validate:signed-monetary val buf))
	#f

	(if (not (string? val))
	    #t ; the field may be empty ...

	    ;; okay, valid so far, do real test ...
	    (let ()
	      (if (= (string-length val) 0)
		  (set! val "0"))
	      
	      (set! max (if (not max)
			    0  ;- no value assigned yet
			
			    (if (string? max)
				(if (= (string-length max) 0)
				    0
				    (string->number max))
				
				max)))

	      (if (< (string->number val) max)
		  #t
		  #f))))))


