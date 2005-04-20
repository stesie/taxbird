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


(tb:form-register
 ; form's name
 "Umsatzsteuervoranmeldung 2005" 

 ; definition
 (lambda ()
   (tb:eval-file "ustva-2005.scm")
   ustva-2005:definition)

 ; retrieval function
 (lambda (buffer element)
   (storage:retrieve buffer element))

 ; storage function
 (lambda (buffer element value)
   (storage:store buffer element value)
   (ustva-2005:recalculate buffer element value))

 ; export function
 (lambda (buf)
   (tb:eval-file "export.scm")
   (export:make-elster-xml
    (export:make-transfer-header buf "UStVA" #t)
    (export:make-nutzdaten-header buf)
    (export:make-steuerfall buf "UStVA" "200501" (ustva-2005:export buf))))

 ; empty set
 (lambda () '(("vend-id" . "74931"))))

