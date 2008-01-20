;;               -*- mode: Scheme; coding: utf-8 -*-
;;
;; Copyright(C) 2005,2007 Stefan Siegl <stesie@brokenpipe.de>
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

; validation func bound to text entries where the user is expected to enter
; the id under which the declaration shall be filed ...
(define steuernummer:validate
  (lambda (value buf)
    ;; value is the tax-id as specified by the user

    (let ((land (storage:retrieve buf "land")))
      (and value
	   land
	   (string->number land)

	   ;; try converting to elster formated tax id, if it fails,
	   ;; returns #f
	   (steuernummer:convert (string->number land) value)
	   #t))))  ;;; this makes sure, we return #t if valid, and not the
                   ;;; return value of steuernummer:convert, i.e. the tax-id





;; convert the tax-id (as entered by the user) to elster compatible format
(define steuernummer:convert
  (lambda (land entered-id)
    (let ((split-id "")
	  ; the tax-ids are usually split into 3 pieces, expect for 
	  ;  - Baden-Wuerttemberg which has 2 pieces
	  ;  - Rheinland-Pfalz which has 4 pieces 
	  (pieces (if (= land 10) 4
	              (if (= land 0) 2 3))))

      ; the id may be split either by spaces or slashes, simply try it out
      (set! split-id (string-split entered-id #\/))

      (if (not (= (length split-id) pieces))
	  ; try splitting by spaces ...
	  (set! split-id (string-split entered-id #\ )))

      (and (= (length split-id) pieces)
	   (steuernummer:convert:splitted land split-id)))))



(define steuernummer:length-expectations
  '((5 5)      ; Baden Württemberg
    (3 3 5)    ; Bayern
    (2 3 5)    ; Berlin
    (3 3 5)    ; Brandenburg
    (2 3 5)    ; Bremen
    (2 3 5)    ; Hamburg
    (3 3 5)    ; Hessen
    (3 3 5)    ; Mecklenburg-Vorpommern
    (2 3 5)    ; Niedersachsen
    (3 4 4)    ; Nordrhein-Westfalen
    (2 3 4 1)  ; Rheinland-Pfalz
    (3 3 5)    ; Saarland
    (3 3 5)    ; Sachsen
    (3 3 5)    ; Sachsen-Anhalt
    (2 3 5)    ; Schleswig-Holstein
    (3 3 5)))  ; Thüringen


(define steuernummer:help-dialog
  (lambda (buf)
    (tb:dlg-info (steuernummer:help #f buf))))
    
(define steuernummer:help
  (lambda (val buf)
    (let ((land (storage:retrieve buf "land")))
      (if (and (string? land) (> (string-length land) 0))
	  (let ((specs (list-ref steuernummer:length-expectations (string->number land)))
		(example ""))

	    ;; we got e.g. '(3 3 5) as specs now ...
	    (while (> (length specs) 0)
		   (set! example (string-append 
				  example
				  (substring "12345" 0 (car specs))))
		   (set! specs (cdr specs))
		   (if (> (length specs) 0)
		       (set! example (string-append example "/"))))

	    (format #f (string-append "Die Eingabe der Steuernummer muss "
				      "in der Form eingegeben werden, wie "
				      "diese auf dem Steuerbescheid "
				      "abgedruckt ist (inklusive der "
				      "Finanzamtsnummer).\n\n"
				      "Zum Beispiel: ~A\n\n"
				      "(Alternativ kann die Eingabe durch "
				      "Leerzeichen getrennt erfolgen)")
		    example))

	  ;; else ... (no land chosen)
	  (string-append "Bitte wählen Sie vor Eingabe "
			 "der Steuernummer das Bundesland "
			 "aus, welches für die Verarbeitung "
			 "der Daten zuständig ist.")))))



; do further validation on the splitted fields and form the new tax id
(define steuernummer:convert:splitted
  (lambda (land split-id)
    (let ((valid #t)
	  (length-check-id split-id)
	  (length-expectation (list-ref steuernummer:length-expectations land))
	  (prefixes '(#f "9" "11" "3" "24" "22" "26" "4" "23"
			 "5" "27" "1" "3" "3" "21" "4"))
	  (prefix #t))

      (set! prefix (list-ref prefixes land))

      (while (> (length length-expectation) 0)
	     (if (not (= (car length-expectation)
			 (string-length (car length-check-id))))
		 (set! valid #f))

	     (set! length-expectation (cdr length-expectation))
	     (set! length-check-id (cdr length-check-id)))

      (and valid
	   (if (not prefix)
	       ;; special combination routine for ba-wue
	       (string-append "28" (substring (car split-id) 0 2)
			      "0" (substring (car split-id) 2)
			      (cadr split-id))
	      
	       (string-append prefix
			      (substring (car split-id)
					 (- (string-length (car split-id))
					    (- 4 (string-length prefix))))
			     
			      "0"
			      (cadr split-id)
			      (caddr split-id)
			      (if (= land 10) (cadddr split-id) "")))))))

	    
