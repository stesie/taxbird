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

(define storage:retrieve
  (lambda (buffer element)
    (if (zero? (length buffer))
	#f ; sorry, cannot help no more data available

	(if (string=? (caar buffer) element)
	    (cdar buffer)
	    (storage:retrieve (cdr buffer) element)))))



(define storage:store
  (lambda (buffer element value)
    (if (number? value) (set! value (number->string value)))
    (storage:store__ buffer element value)
    ;(write buffer)(newline)
    ))

(define storage:store__
  (lambda (buffer element value)
    ;;; FIXME: we expect buffer to always contain at least one element here!

    (if (string=? (caar buffer) element)
	(set-cdr! (car buffer) value)

	(if(= 1 (length buffer))
	   (set-cdr! buffer (list (cons element value)))
	   (storage:store__ (cdr buffer) element value)))))
