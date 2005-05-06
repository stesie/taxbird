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


;; revalidate:buffer
;; try to revalidate the whole buffer (against the sheets definition tree
;; 'def'), emiting a suitable error message if necessary
;; RETURN: #t on success, #f on any error
(define revalidate:buffer 
  (lambda (def buf)
    (let ((ret-val #t))
      (while (> (length def) 0)
	     ;;(format #t "revalidating sheet ~A\n" (car def))
	     
	     (if (number? (car (cadr def)))
		 (if (not (revalidate:sheet (car def) (cadr def) buf))
		     (set! ret-val #f))

		 ;; another sheet, recurse downwards ...
		 (if (not (revalidate:buffer (cadr def) buf))
		     (set! ret-val #f)))

	     (if (not ret-val)
		 (set! def '())           ; return immediately ...

		 (set! def (cddr def))))  ; forward definition

      ret-val)))


(define revalidate:sheet
  (lambda (sheetname sheetdef buf)
    (if (not (number? (car sheetdef)))
	(scm-error 'wrong-type-arg #f "car of sheetdef must be a number: ~S"
		   (car sheetdef) #f))

    (set! sheetdef (cdr sheetdef)) ; skip the number of columns ...

    (let ((ret-val #t))
      (while (and ret-val
		  (> (length sheetdef) 0))

	     ;;(format #t "caring for row ~S\n" (car (car sheetdef)))

	     (let ((rowdef (cdr (car sheetdef))))
	       (while (and ret-val
			   (> (length rowdef) 0))
			   
		      ;;(write (cadddr rowdef)) (newline)

		      (let ((valid (cadddr rowdef))
			    (value (storage:retrieve buf (cadr rowdef))))

			(if (procedure? valid)
			    (set! valid (valid value buf)))

			(if (list? valid)
			    (set! valid
				  (if value #t #f)))

			(if (not (boolean? valid))
			    (tb:dlg-error
			     (format #f (string-append
					 "Die Validierungsroutine des Feldes "
					 "~S ist nicht gültig: ~S")
				     (cadr rowdef) valid)))

			(if (not valid)
			    (let ()
			      (tb:dlg-error
			       (format #f (string-append
					   "Auf der Seite ~S sind nicht "
					   "alle Feldinhalte gültig:\n\n~A")
				       
				       sheetname (caddr rowdef)))
			      (set! ret-val #f))))

		      (set! rowdef (cddddr rowdef))))
			
	     (set! sheetdef (cdr sheetdef)))

      ret-val)))
	       
	     
	     