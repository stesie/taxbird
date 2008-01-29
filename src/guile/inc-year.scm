;; Copyright(C) 2008 Stefan Siegl <stesie@brokenpipe.de>
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

(use-modules (ice-9 regex))

(define inc-year:ustva-regexp
  (make-regexp "^Umsatzsteuervoranmeldung"))

(define inc-year:year-regexp 
  (make-regexp "(200[6-9])"))

(define tb:inc-year
  (lambda (data)
    (let ((buf        (cadr data))
	  (match      (regexp-exec inc-year:year-regexp (car data)))
	  (template   "")
	  (year       #f))

      ;; extract and bump year ...
      (set! year      (match:substring match))
      (set! template  (string-append
		       (match:prefix match)
		       (number->string (+ 1 (string->number year)))))
      (set-car! data template)

      ;; FIXME we need to adjust for some tax law changes
      ;; like change of VAT rate ..

    ;; seems like we're done
    )))