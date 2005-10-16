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


;; revalidate:buffer
;; try to revalidate the whole buffer (against the sheets definition tree
;; 'def'), emiting a suitable error message if necessary
;; RETURN: #t on success, #f on any error
(define revalidate:buffer 
  (lambda (validator buf)
    (let ((rv #t) (copy buf))
      (while (> (length buf) 0)
	     (let ((validator-result (validator copy (caar buf) (cdar buf))))
	       (if (not validator-result)
		   (tb:dlg-error
		    (format #f "Der Inhalt des Feldes ~S ist ungültig: ~S"
			    (caar buf) (cdar buf))))

	       (set! rv (and rv validator-result)))
	     (set! buf (cdr buf)))

      rv)))
	     