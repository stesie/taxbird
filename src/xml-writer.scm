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

(display (string-append
	  "Guile'd XML-Writer, Version 0.1 *alpha*\n"
	  "Copyright(C) 2005 Stefan Siegl <ssiegl@gmx.de>\n"
	  "This is free software, covered by the GNU General Public License."
	  "\n\n"))

(define xml-writer:write
  (lambda (xml port)
    (if (not (port? port))
	(scm-error 'wrong-type-arg #f "ARG 2, exporting port: ~S"
		   (list port) #f))

    (display "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n" port)
    (xml-writer:write-doit xml port 0)))

(define xml-writer:write-doit
  (lambda (xml port indent)
    (if (not (list? xml))
	(scm-error 'wrong-type-arg #f "ARG 1, expecting list: ~S"
		   (list xml) #f))
    (if (not (port? port))
	(scm-error 'wrong-type-arg #f "ARG 2, exporting port: ~S"
		   (list port) #f))
    (if (not (= (modulo (length xml) 3) 0))
	(scm-error 'wrong-type-arg #f "length of xml-data not modulo 3: ~S (~S)"
		   (list (length xml) xml) #f))

    (while (not (null? xml))
	   (if (not (string? (car xml)))
	       (scm-error 'wrong-type-arg #f "expecting string as car: ~S"
			  (list xml) #f))
	   
	   (display (make-string (* 2 indent) #\ ) port) ; indent starttag
	   (format port "<~A" (car xml)) ; write starttag
	   
	   ; write out attributes (if any) ...
	   (if (list? (cadr xml))
	       (let ((attrs (cadr xml)))
		 (while (not (null? attrs))
			(format port " ~A=~S" (caar attrs) (cadar attrs))
			(set! attrs (cdr attrs)))))
	   
	   (if (not (caddr xml))
	       (display "/" port)) ; no content specified, close immediately
	   
	   (display ">" port) ; end of starttag
	   
	   (if (string? (caddr xml))
	       (display (caddr xml) port)
	       (if (list? (caddr xml))
		   (let ()
		     (newline port)
		     (xml-writer:write-doit (caddr xml) port (+ indent 1))
		     (display (make-string (* 2 indent) #\ ) port))))
	   
	   (if (caddr xml)
	       (format port "</~A>" (car xml))) ; write endtag
	   
	   (newline port)
	   ; forward xml-data list and continue ...
	   (set! xml (cdddr xml)))))
