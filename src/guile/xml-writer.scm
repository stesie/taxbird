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
	  "Guile'd XML-Writer, Version 0.2\n"
	  "Copyright(C) 2005 Stefan Siegl <stesie@brokenpipe.de>\n"
	  "This is free software, covered by the GNU General Public License."
	  "\n\n"))

(define xml-writer:write
  (lambda (xml port)
    (if (not (port? port))
	(scm-error 'wrong-type-arg #f "ARG 2, exporting port: ~S"
		   (list port) #f))

    (display (xml-writer:export xml) port)))

(define xml-writer:export
  (lambda (xml)
    (string-append "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n"
		   (xml-writer:write-doit xml 0))))

(define xml-writer:write-doit
  (lambda (xml indent)
    ;; load string-replace function needed by use-entities ...
    (tb:eval-file "string.scm")

    (if (not (list? xml))
	(scm-error 'wrong-type-arg #f "ARG 1, expecting list: ~S"
		   (list xml) #f))
    (if (not (= (modulo (length xml) 3) 0))
	(scm-error 'wrong-type-arg #f "length of xml-data not modulo 3: ~S (~S)"
		   (list (length xml) xml) #f))

    (let ((result ""))
      (while (not (null? xml))
	     (if (not (string? (car xml)))
		 (scm-error 'wrong-type-arg #f "expecting string as car: ~S"
			    (list xml) #f))
	   
	     (set! result
		   (string-append result
				  (make-string (* 2 indent) #\ ) ; indent!
				  (format #f "<~A" (car xml)))) ; starttag
	   
	     ;; write out attributes (if any) ...
	     (if (list? (cadr xml))
		 (let ((attrs (cadr xml)))
		   (while (not (null? attrs))
			  (set! result
				(string-append result
					       (format #f " ~A=~S"
						       (caar attrs)
						       (cadar attrs))))
			  (set! attrs (cdr attrs)))))
	   
	     (if (not (caddr xml))
		 ;; no content specified, close immediately
		 (set! result (string-append result "/")))
	   
	     ;; end of starttag
	     (set! result (string-append result ">"))

	   
	     (if (string? (caddr xml))
		 (set! result (string-append result 
					     (use-entities (caddr xml))))

		 ;; not a string, recurse to handle child elements ...
		 (if (list? (caddr xml))
		     (let ()
		       (set! result
			     (string-append result "\n"
					    (xml-writer:write-doit (caddr xml)
								   (+ indent 1))
					    (make-string (* 2 indent) #\ ))))))
	   
	     (if (caddr xml)
		 (set! result
		       (string-append result
				      (format #f "</~A>" (car xml))))) ; endtag
	   
	     (set! result (string-append result "\n"))

	     ;; forward xml-data list and continue ...
	     (set! xml (cdddr xml)))

      ;; return result
      result)))


(define use-entities 
  (lambda (str)
    (set! str (string-replace str #\& "&amp;"))
    (set! str (string-replace str #\< "&lt;"))
    (set! str (string-replace str #\> "&gt;"))

    str))

