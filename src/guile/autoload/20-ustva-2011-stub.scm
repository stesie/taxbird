;; vim:ts=8:sw=2:sts=2:noet
;;
;; Copyright(C) 2004,2005,2006,2007,2008,2009,2011 Stefan Siegl <stesie@brokenpipe.de>
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

(tb:eval-file "versions.scm")
(tb:eval-file "mandverw.scm")

(and 
 (require-version 0 4 "Umsatzsteuervoranmeldung 2011")
 (require-geier-version 0 12 "Umsatzsteuervoranmeldung 2011")

 ;; if version requirement is met, try to add the sheet ...
 (tb:form-register
  ;; form's name
  "Umsatzsteuervoranmeldung 2011" 

  ;; get sheet tree -----------------------------------------------------------
  (lambda ()
    (tb:eval-file "ustva-2010.scm")
    (tb:eval-file "ustva-2011.scm")
    (tb:eval-file "sheettree.scm")
    (extract-sheet-tree ustva-2011:definition))



  ;; get sheet ----------------------------------------------------------------
  (lambda (sheetname)
    (tb:eval-file "ustva-2010.scm")
    (tb:eval-file "ustva-2011.scm")
    (ustva-2011:get-sheet sheetname))



  ;; retrieval function -------------------------------------------------------
  (lambda (buffer element)
    (storage:retrieve buffer element))



  ;; storage function ---------------------------------------------------------
  (lambda (buffer element value)
    (or (datenlieferant:button-demux buffer element)
	(mandverw:demux buffer element value)

	(let ((validity (and (ustva-2010:validate buffer element value)
			     (datenlieferant:validate buffer element value))))
	  (if validity
	      (let ()
		(if (and (= (string-length element) 4)
			 (string=? (substring element 0 2) "Kz"))

		    ;; it's an Kz?? entry, therefore pretty print it and 
		    ;; afterwards recalculate the sheet ...
		    (let ((non-floats (list "Kz35" "Kz41" "Kz42" "Kz43" "Kz44"
					    "Kz45" "Kz48" "Kz49" "Kz51" "Kz52"
					    "Kz60" "Kz76" "Kz73" "Kz77" "Kz84"
					    "Kz86" "Kz91" "Kz93" "Kz94" "Kz95"
					    "Kz97"
					    ;; checkboxes ...
					    "Kz10" "Kz26" "Kz29" "Kz22"
					    )))

		      (storage:store buffer element
				     (number->monetary-string 
				      (not (member element non-floats)) value))

		      ;; call the recalculation function ...
		      (ustva-2010:recalculate buffer element value))


		    ;; not a Kz-field, just store and do nothing ...
		    (storage:store buffer element value))))
		
	  
	  validity)))

  ;; recalculation function ---------------------------------------------------
  (lambda (buf)
    (tb:eval-file "ustva-2010.scm")
    (ustva-2010:recalculate buf "" ""))

  ;; export function ----------------------------------------------------------
  (lambda (buf test)
    (tb:eval-file "revalidate.scm")
    (if (and (revalidate:buffer ustva-2010:validate buf ustva-2010:validators)
	     (revalidate:buffer datenlieferant:validate buf 
				datenlieferant:validators))

	(let ((sig-result (if test #f "00616")))

	  ;; document's content is valid, let's export it, to make the
	  ;; IRO know, what nice programs there exist out in the free world ...
	  (tb:eval-file "export.scm")
	  (export:make-elster-xml
	   (export:make-transfer-header buf "UStVA" sig-result)
	   (export:make-nutzdaten-header buf sig-result)
	   (ustva-2011:export-steuerfall buf sig-result)))))



  ;; empty set ----------------------------------------------------------------
  (lambda () 
    (tb:eval-file "datenlieferant.scm")
    (tb:eval-file "mandverw.scm")

    (append (mandverw:return-defaults)
	    datenlieferant:defaults
	    '(("vend-id" . "74931") 
	      ("land-lieferant" . "Deutschland"))))))
		 


