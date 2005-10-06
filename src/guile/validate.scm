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

(tb:eval-file "monetary.scm")


(define validate:alphanum
  (lambda (value min max)
    (let ((length (if (string? value) (string-length value) 0)))
      (and (>= length min)
	   (<= length max)))))
           ;;; FIXME! add some checks, making sure that 
           ;;; there are alphanumeric characters only ....



;; make sure the specified value is a signed integer
(define validate:signed-int
  (lambda (value buffer)
    (and (or (not (string? value))
	     (= (string-length value) 0)

	     (let ((conv-val (ms->number value)))
	       (and (number? conv-val)
		    (integer? conv-val))))

	 #t))) ; make sure to return #t not (not #f) !!



(define validate:unsigned-int
  (lambda (value buffer)
    (and (or (not (string? value)) 
	     (= (string-length value) 0)

	     (and (validate:signed-int value buffer)
		  (>= (ms->number value) 0)))

	 #t))) ; make sure to return #t if valid, not (not #f) !!



;; make sure the specified value is a signed monetary value, i.e. not more 
;; than two cent digits
(define validate:signed-monetary
  (lambda (value buffer)
    (and (or (not (string? value))
	     (= (string-length value) 0)

	     (let ((conv-val value))
	       (if (string-index conv-val #\,)

		   ;; maybe some german locale number?   e.g.  1.234,56
		   (let ()
		     (tb:eval-file "string.scm")
		     (set! conv-val (string-replace conv-val #\. ""))
		     (set! conv-val (string-replace conv-val #\, "."))))

	       (and (string->number conv-val)  ;; NaN ??
		    (let ((split-val (string-split conv-val #\.)))
		      (or 
		       ;; if there is no comma, it's alright ...
		       (= (length split-val) 1)

		       ;; if there is _one_ comma, it might be okay ...
		       (and (= (length split-val) 2)
			   ;; ... if the second string is not more 
			   ;; than 2 digits long
			    (<= (string-length (cadr split-val)) 2)))))))

	 #t))) ;; make sure to return #t if valid.




;; validation function for fields where you can enter the turnover and the
;; tax on that turnover. where 'max' is the turnover, buf the storage buffer
;; and 'val' the tax for the turnover as specified
;;
;; allowed values are (max == val == 0) and (val != 0 && val < max && max != 0)
(define validate:signed-monetary-max
  (lambda (val buf max)
    (if (string? max) (set! max (ms->number max)))
    (if (not max) (set! max 0)) ;;; max may be #f, if it is either not yet
                                ;;; stored or was not a number, i.e. "" ...

    (and (validate:signed-monetary val buf)

	 (let ()
	   (set! val (if (or (not (string? val))
			     (= (string-length val) 0)) "0" val))
	   (set! val (ms->number val))

	   (if (= max 0)
	       ;; if max is set zero (or empty), don't allow associated field
	       ;; to have a value assigned (i.e. force it to zero)
	       (= val 0)

	       ;; otherwise, if max is set, don't allow zero and force 
	       ;; value to be less than 'max', since the tax you have to pay
	       ;; for something you earned, shouldn't be 100% ;-)
	       (and (< val max) 
		    (not (= val 0))))))))
