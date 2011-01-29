;; Copyright(C) 2004,2005,2006,2007,2008,2011 Stefan Siegl <stesie@brokenpipe.de>
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
 (require-version 0 4 "Antrag auf Dauerfristverlängerung 2011")
 (require-geier-version 0 12 "Antrag auf Dauerfristverlängerung 2011")
 
 ;; if version requirement is met, try to add the sheet ...
 (tb:form-register
  ;; form's name
  "Antrag auf Dauerfristverlängerung 2011" 

  ;; get sheet tree -----------------------------------------------------------
  (lambda ()
    (tb:eval-file "ustsvza-2007.scm")
    (tb:eval-file "sheettree.scm")
    (extract-sheet-tree ustsvza-2007:definition))



  ;; get sheet ----------------------------------------------------------------
  (lambda (sheetname)
    (tb:eval-file "ustsvza-2007.scm")
    (ustsvza-2007:get-sheet sheetname))



  ;; retrieval function -------------------------------------------------------
  (lambda (buffer element)
    (storage:retrieve buffer element))



  ;; storage function ---------------------------------------------------------
  (lambda (buffer element value)
    (or (datenlieferant:button-demux buffer element)
	(mandverw:demux buffer element value)

	(let ((validity (and (ustsvza-2007:validate buffer element value)
			     (datenlieferant:validate buffer element value))))
	  (if validity
	      (let ()
		(storage:store buffer element value)
		
		(if (string=? element "ustvz")
		    (ustsvza-2007:recalculate buffer element value))))
	  
	  validity)))


  ;; recalculation function ---------------------------------------------------
  (lambda (buf)
	#t)

  ;; export function ----------------------------------------------------------
  (lambda (buf test)
    (tb:eval-file "revalidate.scm")
    (tb:eval-file "ustsvza-2011.scm")
    (if 
     (and (revalidate:buffer ustsvza-2007:validate buf ustsvza-2007:validators)
	  (revalidate:buffer datenlieferant:validate buf 
			     datenlieferant:validators))

	(let ((sig-result (if test #f "00616")))

	  ;; document's content is valid, let's export it, to make the
	  ;; IRO know, what nice programs there exist out in the free world ...
	  (tb:eval-file "export.scm")
	  (export:make-elster-xml
	   (export:make-transfer-header buf "UStVA" sig-result)
	   (export:make-nutzdaten-header buf sig-result)
	   (ustsvza-2011:export-steuerfall buf sig-result)))))



  ;; empty set ----------------------------------------------------------------
  (lambda () 
    (tb:eval-file "datenlieferant.scm")
    (tb:eval-file "mandverw.scm")

    (append (mandverw:return-defaults)
	    datenlieferant:defaults
	    '(("Umsatzsteuersondervorauszahlung" . 
	       "Umsatzsteuersondervorauszahlung") 
	      ("Kz38" . "0")
	      ("land-lieferant" . "Deutschland"))))))
		 


