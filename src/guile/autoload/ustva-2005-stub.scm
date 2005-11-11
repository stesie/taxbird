;; Copyright(C) 2004,05 Stefan Siegl <ssiegl@gmx.de>
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

(tb:eval-file "versions.scm")

(and 
 (require-version 0 4 "Umsatzsteuervoranmeldung 2005")

 ;; if version requirement is met, try to add the sheet ...
 (tb:form-register
  ;; form's name
  "Umsatzsteuervoranmeldung 2005" 

  ;; get sheet tree -----------------------------------------------------------
  (lambda ()
    (tb:eval-file "ustva-2005.scm")
    (tb:eval-file "sheettree.scm")
    (extract-sheet-tree ustva-2005:definition))



  ;; get sheet ----------------------------------------------------------------
  (lambda (sheetname)
    (tb:eval-file "ustva-2005.scm")
    (ustva-2005:get-sheet sheetname))



  ;; retrieval function -------------------------------------------------------
  (lambda (buffer element)
    (storage:retrieve buffer element))



  ;; storage function ---------------------------------------------------------
  (lambda (buffer element value)
    (or (datenlieferant:button-demux buffer element)

	(let ((validity (and (ustva-2005:validate buffer element value)
			     (datenlieferant:validate buffer element value))))
	  (if validity
	      (let ()
		(storage:store buffer element value)
		
		;; if the stored value is Kz?? call the recalculation function
		(if (and (= (string-length element) 4)
			 (string=? (substring element 0 2) "Kz"))
		    (ustva-2005:recalculate buffer element value))))
	  
	  validity)))


  ;; export function ----------------------------------------------------------
  (lambda (buf)
    (ustva-2005:recalculate buf "" "")

    (tb:eval-file "revalidate.scm")
    (if (and (revalidate:buffer ustva-2005:validate buf)
	     (revalidate:buffer datenlieferant:validate buf))
	(let ((sig-result (tb:check-sig "signatures/ustva-2005.sig")))

	  ;; document's content is valid, let's export it, to make the
	  ;; IRO know, what nice programs there exist out in the free world ...
	  (tb:eval-file "export.scm")
	  (export:make-elster-xml
	   (export:make-transfer-header buf "UStVA" sig-result)
	   (export:make-nutzdaten-header buf sig-result)
	   (export:make-steuerfall buf "UStVA" "200501"
				   (ustva-2005:export buf sig-result))))))



  ;; empty set ----------------------------------------------------------------
  (lambda () 
    (append datenlieferant:defaults
	    '(("vend-id" . "74931") 
	      ("land-lieferant" . "Deutschland"))))))
		 


