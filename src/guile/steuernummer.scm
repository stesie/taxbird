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


; validation func bound to text entries where the user is expected to enter
; the id under which the declaration shall be filed ...
(define steuernummer:validate
  (lambda (value buf)
    ;; value is the tax-id as specified by the user

    (let ((land (string->number (storage:retrieve buf "land"))))

      (if (not land)
	  #f ; federal state not yet specified, we're not able to verify
	     ; the entered value this way.

	  ; try converting to elster formated tax id, if it fails,
	  ; steuernummer:convert returns #f which would be passed on then.
	  (if (steuernummer:convert land value)
	      #t
	      #f)))))



;; convert the tax-id (as entered by the user) to elster compatible format
(define steuernummer:convert
  (lambda (land entered-id)
    (let ((split-id "")
	  ; the tax-ids are usually split into 3 pieces, expect for BaWue
	  ; (land id 0) where there are only 2 pieces ....
	  (pieces (if (= land 0) 2 3)))

      ; the id may be split either by spaces or slashes, simply try it out
      (set! split-id (string-split entered-id #\/))

      (if (not (= (length split-id) pieces))
	  ; try splitting by spaces ...
	  (set! split-id (string-split entered-id #\ )))

      (if (= (length split-id) pieces)
	  (steuernummer:convert:splitted land split-id)

	  #f ; cannot split id into three pieces, sorry ...
	  ))))


; do further validation on the splitted fields and form the new tax id
(define steuernummer:convert:splitted
  (lambda (land split-id)
    (let ((valid #t)
	  (length-check-id split-id)
	  (length-expectation '((5 5)      ; Baden Württemberg
				(3 3 5)    ; Bayern
				(2 3 5)    ; Berlin
				(3 3 5)    ; Brandenburg
				(2 3 5)    ; Bremen
				(2 3 5)    ; Hamburg
				(3 3 5)    ; Hessen
				(3 3 5)    ; Mecklenburg-Vorpommern
				(2 3 5)    ; Niedersachsen
				(3 4 4)    ; Nordrhein-Westfalen
				(2 3 5)    ; Rheinland-Pfalz
				(3 3 5)    ; Saarland
				(3 3 5)    ; Sachsen
				(3 3 5)    ; Sachsen-Anhalt
				(2 3 5)    ; Schleswig-Holstein
				(3 3 5)))  ; Thüringen
	  (prefixes '(#f "9" "11" "3" "24" "22" "26" "4" "23"
			 "5" "27" "1" "3" "3" "21" "4"))
	  (prefix #t))

      ; forward to the length-expectation slot we need
      (while (< 0 land)
	     (set! length-expectation (cdr length-expectation))
	     (set! prefixes (cdr prefixes))
	     (set! land (- land 1)))

      (set! length-expectation (car length-expectation))
      (set! prefix (car prefixes))

      (while (> (length length-expectation) 0)
	     (if (not (= (car length-expectation)
			 (string-length (car length-check-id))))
		 (set! valid #f))

	     (set! length-expectation (cdr length-expectation))
	     (set! length-check-id (cdr length-check-id)))

      (if (not valid)
	  #f ; return error ...
	  
	  ;; special combination routine for ba-wue
	  (if (not prefix)
	      (string-append "28" (substring (car split-id) 0 2)
			     "0" (substring (car split-id) 2)
			     (cadr split-id))
	      
	      (string-append prefix
			     (substring (car split-id)
					(- (string-length (car split-id))
					   (- 4 (string-length prefix))))
			     
			     (make-string (- 4 (string-length
						(cadr split-id)))
					  #\0)
			     (cadr split-id)
			     
			     (caddr split-id)))))))

	    
