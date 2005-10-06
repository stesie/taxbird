;; Copyright(C) 2005 Stefan Siegl <stesie@brokenpipe.de>
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