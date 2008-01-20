;; Copyright(C) 2005,2006,2007 Stefan Siegl <stesie@brokenpipe.de>
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

;; we use the ice-9 pretty printer to reformat our numbers ...
(use-modules (ice-9 format))


(define monetary-string->number
  (lambda (in)
    (if (string-index in #\,)

	;; maybe some german locale number?   e.g.  1.234,56
	(let ()
	  (tb:eval-file "string.scm")
	  (set! in (string-replace in #\. ""))
	  (set! in (string-replace in #\, "."))))

    (string->number in)))


;; some kind of shortcut for the monetary-string->number function 
(define ms->number monetary-string->number)


(define dotify
  (lambda (in)
    (tb:eval-file "string.scm")

    (set! in (string-replace in #\. ","))

    (let ((pos (string-index in #\,)))
      (if (not pos)
	  (set! pos (string-length in)))

      (while (> pos 3)
	     (set! pos (- pos 3))
	     (set! in (string-append (substring in 0 pos) "."
				     (substring in pos)))))

    in))


(define number->monetary-string
  (lambda (float val)
    (if (string? val) (set! val (ms->number val)))

    (if val
	;;(dotify
	;;
	;; don't dotify the output for the moment, in order to not break
	;; backward compatibility. so far numbers with thousands delimitting
	;; dots haven't been accepted, since these have been
	;; treated as commas. However I am quite sure that many
	;; users are still used to entering a dot as a comma
	;; (which used to be a necessity for quite a long time)
	;; and therefore would get disappointed if we forbid to do
	;; this now.
	;;
	(if float
	    (let ()
	      (tb:eval-file "string.scm")
	      (string-replace (format #f "~,2F" (exact->inexact val)) #\. ","))

	    ;; convert to non floating point number ...
	    ;; call inexact->exact to strip a trailing .00 from the integer
	    (format #f "~D" (inexact->exact val)))
    
	;; no valid (aka true) input value ...
	#f)))
